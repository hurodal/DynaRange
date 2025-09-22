///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/filepicker.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/gauge.h>
#include <wx/statbmp.h>
#include <wx/notebook.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MyFrameBase
///////////////////////////////////////////////////////////////////////////////
class MyFrameBase : public wxFrame
{
	private:

	protected:
		wxNotebook* m_mainNotebook;
		wxPanel* m_inputPanel;
		wxStaticText* m_darkFileStaticText;
		wxFilePickerCtrl* m_darkFilePicker;
		wxStaticText* m_darkValueStaticText;
		wxTextCtrl* m_darkValueTextCtrl;
		wxStaticText* m_saturationFileStaticText;
		wxFilePickerCtrl* m_saturationFilePicker;
		wxStaticText* m_saturationValueStaticText;
		wxTextCtrl* m_saturationValueTextCtrl;
		wxStaticText* m_patchRatioStaticText;
		wxSlider* m_patchRatioSlider;
		wxStaticText* m_patchRatioValueText;
		wxStaticText* m_snrThresholdStaticText;
		wxSlider* m_snrThresholdslider;
		wxStaticText* m_snrThresholdValueText;
		wxStaticText* m_drNormalizationStaticText;
		wxSlider* m_drNormalizationSlider;
		wxStaticText* m_drNormalizationValueText;
		wxStaticText* m_polynomicStaticText;
		wxChoice* m_PlotChoice;
		wxStaticText* m_plotingStaticText;
		wxChoice* m_plotingChoice;
		wxStaticText* m_outputStaticText;
		wxTextCtrl* m_outputTextCtrl;
		wxListBox* m_rawFileslistBox;
		wxButton* m_addRawFilesButton;
		wxButton* m_removeRawFilesButton;
		wxTextCtrl* m_equivalentCliTextCtrl;
		wxButton* m_executeButton;
		wxPanel* m_logPanel;
		wxTextCtrl* m_logOutputTextCtrl;
		wxPanel* m_resultsPanel;
		wxStaticText* m_csvOutputStaticText;
		wxGrid* m_cvsGrid;
		wxStaticText* m_generateGraphStaticText;
		wxGauge* m_processingGauge;
		wxStaticBitmap* m_imageGraph;

	public:

		MyFrameBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Dynamic Range Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 700,800 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MyFrameBase();

};

