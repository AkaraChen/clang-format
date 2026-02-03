// Simple C-style exports for WASM without embind
// This allows the WASM to be called from pure Rust (wasmi) without JS glue

#include "lib.h"
#include <cstring>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

// Global formatter instance
static ClangFormat* g_formatter = nullptr;

// Result structure in linear memory
struct WasmResult {
    int32_t status;      // 0 = Success, 1 = Error, 2 = Unchanged
    char* content_ptr;
    int32_t content_len;
};

static WasmResult g_last_result = {0, nullptr, 0};

// Memory management helpers - these will be called from Rust
extern "C" {

// Allocate memory in WASM linear memory
WASM_EXPORT
void* wasm_alloc(size_t size) {
    return malloc(size);
}

// Free memory in WASM linear memory
WASM_EXPORT
void wasm_dealloc(void* ptr) {
    free(ptr);
}

// Initialize the formatter
WASM_EXPORT
void wasm_init() {
    if (g_formatter == nullptr) {
        g_formatter = new ClangFormat();
    }
}

// Set style (returns 0 on success)
WASM_EXPORT
int wasm_set_style(const char* style, int style_len) {
    if (g_formatter == nullptr) return -1;
    std::string s(style, style_len);
    g_formatter->with_style(s);
    return 0;
}

// Set fallback style (returns 0 on success)
WASM_EXPORT
int wasm_set_fallback_style(const char* style, int style_len) {
    if (g_formatter == nullptr) return -1;
    std::string s(style, style_len);
    g_formatter->with_fallback_style(s);
    return 0;
}

// Format code - stores result in global, returns status
// 0 = Success, 1 = Error, 2 = Unchanged
WASM_EXPORT
int wasm_format(const char* code, int code_len, 
                const char* filename, int filename_len) {
    if (g_formatter == nullptr) {
        g_last_result.status = 1; // Error
        g_last_result.content_ptr = nullptr;
        g_last_result.content_len = 0;
        return 1;
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
    
    return g_last_result.status;
}

// Get result content pointer from last format call
WASM_EXPORT
char* wasm_get_result_ptr() {
    return g_last_result.content_ptr;
}

// Get result content length from last format call
WASM_EXPORT
int wasm_get_result_len() {
    return g_last_result.content_len;
}

// Free result content
WASM_EXPORT
void wasm_free_result() {
    if (g_last_result.content_ptr != nullptr) {
        free(g_last_result.content_ptr);
        g_last_result.content_ptr = nullptr;
        g_last_result.content_len = 0;
    }
}

// Get version string pointer
WASM_EXPORT
const char* wasm_version() {
    static std::string ver = ClangFormat::version();
    return ver.c_str();
}

// Get version string length
WASM_EXPORT
int wasm_version_len() {
    static std::string ver = ClangFormat::version();
    return ver.size();
}

} // extern "C"
