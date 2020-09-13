#ifndef FUCKMACRO_H
#define FUCKMACRO_H

// macro magic, used for auto hadler naming from API_VERSION=... defined in api.pro file
// https://stackoverflow.com/questions/1489932/how-to-concatenate-twice-with-the-c-preprocessor-and-expand-a-macro-as-in-arg

// api version, just quoted digits
#define PASTE_1(_name) #_name
#define EVALUATOR_1(_name) PASTE_1(_name)
#define API_VERSION_VALUE EVALUATOR_1(API_VERSION)

// api handler string
#define API_HANDLER_PREFIX api_v_
#define PASTE_2(_prefix, _name) _prefix ## _name
#define EVALUATOR_2(_prefix, _name_name) PASTE_2(_prefix, _name_name)
#define API_HANDLER_NAME EVALUATOR_2(API_HANDLER_PREFIX, API_VERSION)

// api handler quoted string
#define PASTE_3(_name) #_name
#define EVALUATOR_3(_name) PASTE_3(_name)
#define API_HANDLER_NAME_QUOTED EVALUATOR_3(API_HANDLER_NAME)


#define NXWEB_DEFINE_HANDLER_PATCHED(...) \
        nxweb_handler nxweb_ ## API_HANDLER_NAME ## _handler={.name=API_HANDLER_NAME_QUOTED, ## __VA_ARGS__}; \
        static void _nxweb_define_handler_ ## API_HANDLER_NAME() __attribute__ ((constructor)); \
        static void _nxweb_define_handler_ ## API_HANDLER_NAME() { \
          _nxweb_define_handler_base(&nxweb_ ## API_HANDLER_NAME ## _handler); \
        }

#endif // FUCKMACRO_H
