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
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/splitter.h>
#include <wx/grid.h>
#include <wx/gauge.h>
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
		wxButton* m_clearDarkFileButton;
		wxStaticText* m_darkValueStaticText;
		wxTextCtrl* m_darkValueTextCtrl;
		wxStaticText* m_saturationFileStaticText;
		wxFilePickerCtrl* m_saturationFilePicker;
		wxButton* m_clearSaturationFileButton;
		wxStaticText* m_saturationValueStaticText;
		wxTextCtrl* m_saturationValueTextCtrl;
		wxListBox* m_rawFileslistBox;
		wxButton* m_addRawFilesButton;
		wxButton* m_removeRawFilesButton;
		wxButton* m_removeAllFiles;
		wxStaticText* m_patchRatioStaticText;
		wxSlider* m_patchRatioSlider;
		wxStaticText* m_patchRatioValueText;
		wxStaticText* m_drNormalizationStaticText;
		wxSlider* m_drNormalizationSlider;
		wxStaticText* m_drNormalizationValueText;
		wxStaticText* m_snrThresholdStaticText;
		wxTextCtrl* m_snrThresholdsValues;
		wxStaticText* m_polynomicStaticText;
		wxChoice* m_PlotChoice;
		wxStaticText* m_outputStaticText;
		wxTextCtrl* m_outputTextCtrl;
		wxCheckBox* m_saveLog;
		wxStaticText* R_staticText;
		wxCheckBox* R_checkBox;
		wxStaticText* G1_staticText;
		wxCheckBox* G1_checkBox;
		wxStaticText* G2_staticText;
		wxCheckBox* G2_checkBox;
		wxStaticText* B_staticText;
		wxCheckBox* B_checkBox;
		wxStaticText* AVG_staticText;
		wxChoice* AVG_ChoiceValue;
		wxStaticText* chartPatchRowStaticText1;
		wxTextCtrl* m_chartPatchRowValue1;
		wxStaticText* chartPatchColStaticText1;
		wxTextCtrl* m_chartPatchColValue1;
		wxCheckBox* m_debugPatchesCheckBox;
		wxTextCtrl* m_debugPatchesFileNameValue;
		wxCheckBox* m_plotParamScattersCheckBox;
		wxCheckBox* m_plotParamCurveCheckBox;
		wxCheckBox* m_plotParamLabelsCheckBox;
		wxCheckBox* allIsosCheckBox;
		wxStaticText* graphicFormatStaticText;
		wxChoice* m_plotFormatChoice;
		wxStaticText* m_plotingStaticText;
		wxChoice* m_plotingChoice;
		wxButton* m_executeButton;
		wxPanel* m_ChartsManualCorners;
		wxPanel* m_loupePanel;
		wxStaticText* m_staticText38;
		wxTextCtrl* m_coordX1Value;
		wxStaticText* m_staticText381;
		wxTextCtrl* m_coordY1Value;
		wxStaticText* m_staticText42;
		wxTextCtrl* m_coordX2Value;
		wxStaticText* m_staticText43;
		wxTextCtrl* m_coordY2Value;
		wxStaticLine* m_staticline1;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText44;
		wxTextCtrl* m_coordX3Value;
		wxStaticText* m_staticText45;
		wxTextCtrl* m_coordY3Value;
		wxStaticText* m_staticText46;
		wxTextCtrl* m_coordX4Value;
		wxStaticText* m_staticText47;
		wxTextCtrl* m_coordY4Value;
		wxButton* m_clearAllCoordinates;
		wxPanel* m_rawImagePreviewPanel;
		wxSlider* m_gammaThumbSlider;
		wxPanel* m_chartPanel;
		wxSplitterWindow* m_splitterChart;
		wxPanel* leftColChartPanel;
		wxStaticText* m_rParamStaticText;
		wxSlider* m_rParamSlider;
		wxStaticText* m_rParamValue;
		wxStaticText* m_gParamStaticText;
		wxSlider* m_gParamSlider;
		wxStaticText* m_gParamValue;
		wxStaticText* m_bParamStaticText;
		wxSlider* m_bParamSlider;
		wxStaticText* m_bParamValue;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_InvGammaValue;
		wxStaticText* m_chartDimXStaticText;
		wxTextCtrl* m_chartDimXValue;
		wxStaticText* m_chartDimWStaticText;
		wxTextCtrl* m_chartDimWValue;
		wxStaticText* m_chartDimHStaticText;
		wxTextCtrl* m_chartDimHValue;
		wxStaticText* m_chartPatchRowStaticText;
		wxTextCtrl* m_chartPatchRowValue;
		wxStaticText* chartPatchColStaticText;
		wxTextCtrl* m_chartPatchColValue;
		wxButton* chartButtonCreate;
		wxButton* chartButtonPreview;
		wxPanel* rightColChartPanel;
		wxPanel* m_webView2PlaceholderPanel;
		wxPanel* m_logPanel;
		wxTextCtrl* m_logOutputTextCtrl;
		wxPanel* m_equCliPanel;
		wxTextCtrl* m_equivalentCliTextCtrl;
		wxPanel* m_resultsPanel;
		wxSplitterWindow* m_splitterResults;
		wxPanel* m_leftPanel;
		wxStaticText* m_csvOutputStaticText;
		wxGrid* m_cvsGrid;
		wxPanel* m_rightPanel;
		wxStaticText* m_generateGraphStaticText;
		wxGauge* m_processingGauge;
		wxPanel* m_webViewPlaceholderPanel;

	public:

		MyFrameBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Dynamic Range Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MyFrameBase();

		void m_splitterChartOnIdle( wxIdleEvent& )
		{
			m_splitterChart->SetSashPosition( 450 );
			m_splitterChart->Disconnect( wxEVT_IDLE, wxIdleEventHandler( MyFrameBase::m_splitterChartOnIdle ), NULL, this );
		}

		void m_splitterResultsOnIdle( wxIdleEvent& )
		{
			m_splitterResults->SetSashPosition( 0 );
			m_splitterResults->Disconnect( wxEVT_IDLE, wxIdleEventHandler( MyFrameBase::m_splitterResultsOnIdle ), NULL, this );
		}

};

