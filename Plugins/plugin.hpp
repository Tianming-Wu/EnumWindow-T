/*
    Plugin Utility Header

*/

// use WindowControlEx Custom Entry Point feature.
// If this function is found, the plugin system will call the entrypoint instead of letting the plugin
// to load itself.
#define CLAIM_PLUGIN_ENTRYSUPPORT extern "C" __declspec(dllexport) void _i_CEP_() {}

#define PLUGIN_ENTRY extern "C"  __declspec(dllexport)