// Simple C-style exports for WASM without embind
// This allows the WASM to be called from pure Rust (wasmi) without JS glue

#include "lib.h"
#include <cstring>
#include <cstdlib>

// Global formatter instance
static ClangFormat* g_formatter = nullptr;

// Memory management helpers - these will be called from Rust
extern "C" {

// Allocate memory in WASM linear memory
__attribute__((export_name("alloc")))
void* wasm_alloc(size_t size) {
    return malloc(size);
}

// Free memory in WASM linear memory
__attribute__((export_name("dealloc")))
void wasm_dealloc(void* ptr) {
    free(ptr);
}

// Initialize the formatter
__attribute__((export_name("init")))
void wasm_init() {
    if (g_formatter == nullptr) {
        g_formatter = new ClangFormat();
    }
}

// Set style (returns 0 on success)
__attribute__((export_name("set_style")))
int wasm_set_style(const char* style, size_t style_len) {
    if (g_formatter == nullptr) return -1;
    std::string s(style, style_len);
    g_formatter->with_style(s);
    return 0;
}

// Set fallback style (returns 0 on success)
__attribute__((export_name("set_fallback_style")))
int wasm_set_fallback_style(const char* style, size_t style_len) {
    if (g_formatter == nullptr) return -1;
    std::string s(style, style_len);
    g_formatter->with_fallback_style(s);
    return 0;
}

// Result structure in linear memory
// Layout: [status: i32][content_ptr: i32][content_len: i32]
struct WasmResult {
    int32_t status;      // 0 = Success, 1 = Error, 2 = Unchanged
    char* content_ptr;
    int32_t content_len;
};

static WasmResult g_last_result;

// Format code
// Returns pointer to WasmResult struct
__attribute__((export_name("format")))
WasmResult* wasm_format(const char* code, size_t code_len, 
                        const char* filename, size_t filename_len) {
    if (g_formatter == nullptr) {
        g_last_result.status = 1; // Error
        g_last_result.content_ptr = nullptr;
        g_last_result.content_len = 0;
        return &g_last_result;
    }
    
    // Free previous result content if any
    if (g_last_result.content_ptr != nullptr) {
        free(g_last_result.content_ptr);
        g_last_result.content_ptr = nullptr;
    }
    
    std::string code_str(code, code_len);
    std::string filename_str(filename, filename_len);
    
    Result result = g_formatter->format(code_str, filename_str);
    
    switch (result.status) {
        case ResultStatus::Success:
            g_last_result.status = 0;
            break;
        case ResultStatus::Error:
            g_last_result.status = 1;
            break;
        case ResultStatus::Unchanged:
            g_last_result.status = 2;
            break;
    }
    
    if (!result.content.empty()) {
        g_last_result.content_len = result.content.size();
        g_last_result.content_ptr = (char*)malloc(result.content.size());
        memcpy(g_last_result.content_ptr, result.content.c_str(), result.content.size());
    } else {
        g_last_result.content_ptr = nullptr;
        g_last_result.content_len = 0;
    }
    
    return &g_last_result;
}

// Get result status from last format call
__attribute__((export_name("get_result_status")))
int32_t wasm_get_result_status() {
    return g_last_result.status;
}

// Get result content pointer from last format call
__attribute__((export_name("get_result_ptr")))
char* wasm_get_result_ptr() {
    return g_last_result.content_ptr;
}

// Get result content length from last format call
__attribute__((export_name("get_result_len")))
int32_t wasm_get_result_len() {
    return g_last_result.content_len;
}

// Free result content
__attribute__((export_name("free_result")))
void wasm_free_result() {
    if (g_last_result.content_ptr != nullptr) {
        free(g_last_result.content_ptr);
        g_last_result.content_ptr = nullptr;
        g_last_result.content_len = 0;
    }
}

// Get version string
__attribute__((export_name("version")))
const char* wasm_version() {
    static std::string ver = ClangFormat::version();
    return ver.c_str();
}

// Get version string length
__attribute__((export_name("version_len")))
int32_t wasm_version_len() {
    static std::string ver = ClangFormat::version();
    return ver.size();
}

} // extern "C"
