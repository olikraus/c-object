#include <napi.h>
#include "dnfwebapp.h"

static Napi::Value ExecuteDwa(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected (JSON input)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string input_json = info[0].As<Napi::String>().Utf8Value();

    coBVDetect();

    dwa_t p;
    dwa_init(&p);
    p.size = input_json.size();
    p.buffer = (char*)malloc(p.size + 32);
    if (!p.buffer) {
        Napi::Error::New(env, "Failed to allocate memory").ThrowAsJavaScriptException();
        return env.Null();
    }
    memcpy(p.buffer, input_json.data(), p.size);
    p.buffer[p.size] = '\0';
    memset(p.buffer + p.size, 0, 32);

    dwa_scan(&p);
    p.token_idx = 0;

    char *response_json = NULL;
    if (dwa_peek_token_char(&p) == '{') {
        double t1 = get_ms();
        p.psd = coNewPSD();
        p.token_idx = 0;
        dwa_collect_psd_recursive(&p);
        double t2 = get_ms();
        dwa_print(&p, "psd parser time: %.4f ms\n", t2 - t1);

        double t3 = get_ms();
        coBVPreparePSD(p.psd);
        p.bvpos = (co)coMapGet(p.psd, "bvpos");
        p.total_bits = coInt32VectorGet(p.bvpos, coInt32VectorSize(p.bvpos) - 1);
        p.bvmask = (co)coMapGet(p.psd, "bvmask");
        p.bvattributes = (co)coMapGet(p.psd, "bvattributes");
        p.bvvaluepos = (co)coMapGet(p.psd, "bvvaluepos");
        double t4 = get_ms();
        dwa_print(&p, "psd bv prep time: %.4f ms\n", t4 - t3);
        
        double t5 = get_ms();
        p.token_idx = 0;
        dwa_build_bvdnf_recursive(&p, NULL);
        double t6 = get_ms();
        dwa_print(&p, "dnf parser time: %.4f ms\n", t6 - t5);
        
        response_json = dwa_execute_op_to_str(&p);
    } else {
        dwa_validate(&p);
        co wrapper = coNewMap(CO_STRDUP | CO_FREE_VALS);
        coMapAdd(wrapper, "status", (cco)coNewStr(CO_STRDUP, "valid"));
        coMapAdd(wrapper, "log", (cco)coNewStr(CO_STRDUP, p.log_buffer));
        
        size_t size = 0;
        FILE *mem_fp = open_memstream(&response_json, &size);
        if (mem_fp) {
            coWriteJSON(wrapper, 0, 0, mem_fp);
            fclose(mem_fp);
        } else {
            response_json = strdup("{}");
        }
        coDelete(wrapper);
    }

    Napi::String result = Napi::String::New(env, response_json ? response_json : "{}");
    if (response_json) free(response_json);
    dwa_destroy(&p);

    return result;
}

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("execute", Napi::Function::New(env, ExecuteDwa));
    return exports;
}

NODE_API_MODULE(dwa_node, Init)
