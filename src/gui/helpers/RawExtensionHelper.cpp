// File: src/gui/helpers/RawExtensionHelper.cpp
/**
 * @file RawExtensionHelper.cpp
 * @brief Implements the RAW file extension helper utility.
 */
#include "RawExtensionHelper.hpp"
#include "../Constants.hpp"
#include <libraw/libraw.h>
#include <libraw/libraw_version.h>

namespace GuiHelpers {

const std::vector<std::string>& GetSupportedRawExtensions() {
    static std::vector<std::string> extensions;
    if (extensions.empty()) {
        #if defined(LIBRAW_MAJOR_VERSION) && defined(LIBRAW_MINOR_VERSION)
            #if LIBRAW_MAJOR_VERSION > 0 || (LIBRAW_MAJOR_VERSION == 0 && LIBRAW_MINOR_VERSION >= 22)
                LibRaw proc;
                int count = 0;
                const char** ext_list = proc.get_supported_extensions_list(&count);
                if (ext_list) {
                    std::set<std::string> unique;
                    for (int i = 0; i < count; ++i) {
                        if (!ext_list[i]) continue;
                        std::string ext(ext_list[i]);
                        if (!ext.empty() && ext[0] == '.') { ext = ext.substr(1); }
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
                        if (!ext.empty()) { unique.insert(ext); }
                    }
                    extensions.assign(unique.begin(), unique.end());
                }
            #endif
        #endif

        if (extensions.empty()) {
            extensions = DynaRange::Gui::Constants::FALLBACK_RAW_EXTENSIONS;
        }
    }
    return extensions;
}

} // namespace GuiHelpers