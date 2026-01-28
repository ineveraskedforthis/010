#ifdef DCON_LUADLL_EXPORTS
#ifdef _WIN32
#define DCON_LUADLL_API __declspec(dllexport)
#else
#define DCON_LUADLL_API __attribute__((visibility("default")))
#endif
#else
#ifdef _WIN32
#define DCON_LUADLL_API __declspec(dllimport)
#else
#define DCON_LUADLL_API
#endif
#endif