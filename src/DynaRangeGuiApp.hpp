// File: src/DynaRangeGuiApp.hpp
/**
 * @file src/DynaRangeGuiApp.hpp
 * @brief Declares the main application class for the GUI.
 */
#pragma once

#include <wx/app.h>
#include <wx/intl.h>

/**
 * @class DynaRangeGuiApp
 * @brief The main application class for the GUI. Inherits from wxApp.
 */
class DynaRangeGuiApp : public wxApp {
public:
    /**
     * @brief This is the main entry point of the GUI application.
     * It is called on startup.
     * @return True to continue running the application, false to exit.
     */
    virtual bool OnInit() override;

private:
    wxLocale m_locale; ///< Manages the application's language and translations.
};