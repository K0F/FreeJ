#ifndef __JSPARSER_H__
#define __JSPARSER_H__

/*
 * Tune this to avoid wasting space for shallow stacks, while saving on
 * malloc overhead/fragmentation for deep or highly-variable stacks.
 */
#define STACK_CHUNK_SIZE    8192

#include <jsapi.h>
#include <layer.h>

JSBool layer_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool add_layer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool kolos(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

class JsParser {
    public:
	JsParser(Context *_env);
	~JsParser();
	int open(const char* script_file);
	int parse();
    private:
	JSRuntime *js_runtime;
	JSContext *js_context;
	JSObject *global_object;
	void init();
	void init_structs();


	JSClass layer_class;
	JSClass global_class;

	JSFunctionSpec shell_functions[3];
};
#endif
