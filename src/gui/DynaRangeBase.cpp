///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "DynaRangeBase.h"

///////////////////////////////////////////////////////////////////////////

MyFrameBase::MyFrameBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_mainNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_inputPanel = new wxPanel( m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* mainPanelSizer;
	mainPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* darkSaturationSizer;
	darkSaturationSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* darkFrameSbSizer;
	darkFrameSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Dark Frame") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableRow( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_darkFileStaticText = new wxStaticText( darkFrameSbSizer->GetStaticBox(), wxID_ANY, _("Dark File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_darkFileStaticText->Wrap( -1 );
	fgSizer1->Add( m_darkFileStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_darkFilePicker = new wxFilePickerCtrl( darkFrameSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select a file"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	fgSizer1->Add( m_darkFilePicker, 0, wxALL|wxEXPAND, 5 );

	m_darkValueStaticText = new wxStaticText( darkFrameSbSizer->GetStaticBox(), wxID_ANY, _("Dark Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_darkValueStaticText->Wrap( -1 );
	fgSizer1->Add( m_darkValueStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_darkValueTextCtrl = new wxTextCtrl( darkFrameSbSizer->GetStaticBox(), wxID_ANY, _("256.0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_darkValueTextCtrl, 0, wxALL, 5 );


	darkFrameSbSizer->Add( fgSizer1, 1, 0, 5 );


	darkSaturationSizer->Add( darkFrameSbSizer, 1, wxALL, 5 );

	wxStaticBoxSizer* saturationSbSizer;
	saturationSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Saturation") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer2->AddGrowableRow( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_saturationFileStaticText = new wxStaticText( saturationSbSizer->GetStaticBox(), wxID_ANY, _("Saturation File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_saturationFileStaticText->Wrap( -1 );
	fgSizer2->Add( m_saturationFileStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_saturationFilePicker = new wxFilePickerCtrl( saturationSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select a file"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	fgSizer2->Add( m_saturationFilePicker, 0, wxALL|wxEXPAND, 5 );

	m_saturationValueStaticText = new wxStaticText( saturationSbSizer->GetStaticBox(), wxID_ANY, _("Saturation Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_saturationValueStaticText->Wrap( -1 );
	fgSizer2->Add( m_saturationValueStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_saturationValueTextCtrl = new wxTextCtrl( saturationSbSizer->GetStaticBox(), wxID_ANY, _("4095.0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_saturationValueTextCtrl, 0, wxALL|wxEXPAND, 5 );


	saturationSbSizer->Add( fgSizer2, 1, 0, 5 );


	darkSaturationSizer->Add( saturationSbSizer, 1, wxALL, 5 );


	mainPanelSizer->Add( darkSaturationSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* patchRatioSizer;
	patchRatioSizer = new wxBoxSizer( wxHORIZONTAL );

	m_patchRatioStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("Patch Ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_patchRatioStaticText->Wrap( -1 );
	patchRatioSizer->Add( m_patchRatioStaticText, 0, wxALL|wxEXPAND, 5 );

	m_patchRatioSlider = new wxSlider( m_inputPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	patchRatioSizer->Add( m_patchRatioSlider, 1, wxALL|wxEXPAND, 5 );

	m_patchRatioValueText = new wxStaticText( m_inputPanel, wxID_ANY, _("0.50"), wxDefaultPosition, wxDefaultSize, 0 );
	m_patchRatioValueText->Wrap( -1 );
	m_patchRatioValueText->SetMinSize( wxSize( 40,-1 ) );

	patchRatioSizer->Add( m_patchRatioValueText, 0, wxALL|wxEXPAND, 5 );


	mainPanelSizer->Add( patchRatioSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* snrThresholdSizer;
	snrThresholdSizer = new wxBoxSizer( wxHORIZONTAL );

	m_snrThresholdStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("SNR Threshold"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snrThresholdStaticText->Wrap( -1 );
	snrThresholdSizer->Add( m_snrThresholdStaticText, 0, wxALL|wxEXPAND, 5 );

	m_snrThresholdslider = new wxSlider( m_inputPanel, wxID_ANY, 12, 0, 35, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	snrThresholdSizer->Add( m_snrThresholdslider, 1, wxALL|wxEXPAND, 5 );

	m_snrThresholdValueText = new wxStaticText( m_inputPanel, wxID_ANY, _("12dB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snrThresholdValueText->Wrap( -1 );
	snrThresholdSizer->Add( m_snrThresholdValueText, 0, wxALL|wxEXPAND, 5 );


	mainPanelSizer->Add( snrThresholdSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* drNormalizationSizer;
	drNormalizationSizer = new wxBoxSizer( wxHORIZONTAL );

	m_drNormalizationStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("DR Normalization"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drNormalizationStaticText->Wrap( -1 );
	drNormalizationSizer->Add( m_drNormalizationStaticText, 0, wxALL|wxEXPAND, 5 );

	m_drNormalizationSlider = new wxSlider( m_inputPanel, wxID_ANY, 8, 2, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	drNormalizationSizer->Add( m_drNormalizationSlider, 1, wxALL|wxEXPAND, 5 );

	m_drNormalizationValueText = new wxStaticText( m_inputPanel, wxID_ANY, _("8Mpx"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drNormalizationValueText->Wrap( -1 );
	drNormalizationSizer->Add( m_drNormalizationValueText, 0, wxALL|wxEXPAND, 5 );


	mainPanelSizer->Add( drNormalizationSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* plotPolyOutputSizer;
	plotPolyOutputSizer = new wxBoxSizer( wxHORIZONTAL );

	m_polynomicStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("Polynomic Order"), wxDefaultPosition, wxDefaultSize, 0 );
	m_polynomicStaticText->Wrap( -1 );
	plotPolyOutputSizer->Add( m_polynomicStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	wxString m_PlotChoiceChoices[] = { _("Order 2"), _("Order 3") };
	int m_PlotChoiceNChoices = sizeof( m_PlotChoiceChoices ) / sizeof( wxString );
	m_PlotChoice = new wxChoice( m_inputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PlotChoiceNChoices, m_PlotChoiceChoices, 0 );
	m_PlotChoice->SetSelection( 1 );
	plotPolyOutputSizer->Add( m_PlotChoice, 0, wxALL, 5 );


	plotPolyOutputSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_plotingStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("Graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotingStaticText->Wrap( -1 );
	plotPolyOutputSizer->Add( m_plotingStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	wxString m_plotingChoiceChoices[] = { _("Don't Plot"), _("Ploting with CLI commad"), _("Ploting without CLI command") };
	int m_plotingChoiceNChoices = sizeof( m_plotingChoiceChoices ) / sizeof( wxString );
	m_plotingChoice = new wxChoice( m_inputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotingChoiceNChoices, m_plotingChoiceChoices, 0 );
	m_plotingChoice->SetSelection( 1 );
	plotPolyOutputSizer->Add( m_plotingChoice, 0, wxALL, 5 );


	mainPanelSizer->Add( plotPolyOutputSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* outputSizer;
	outputSizer = new wxBoxSizer( wxHORIZONTAL );

	m_outputStaticText = new wxStaticText( m_inputPanel, wxID_ANY, _("Ouput file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputStaticText->Wrap( -1 );
	outputSizer->Add( m_outputStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_outputTextCtrl = new wxTextCtrl( m_inputPanel, wxID_ANY, _("result.csv"), wxDefaultPosition, wxDefaultSize, 0 );
	outputSizer->Add( m_outputTextCtrl, 0, wxALL, 5 );


	mainPanelSizer->Add( outputSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* rawFilesSbSizer;
	rawFilesSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Input RAW Files") ), wxVERTICAL );

	rawFilesSbSizer->SetMinSize( wxSize( -1,150 ) );
	m_rawFileslistBox = new wxListBox( rawFilesSbSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	rawFilesSbSizer->Add( m_rawFileslistBox, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* AddRemoveRawsSbSizer7;
	AddRemoveRawsSbSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_addRawFilesButton = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Add RAW Files..."), wxDefaultPosition, wxDefaultSize, 0 );
	AddRemoveRawsSbSizer7->Add( m_addRawFilesButton, 0, wxALIGN_CENTER|wxALL, 5 );

	m_removeRawFilesButton = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Remove Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeRawFilesButton->Enable( false );

	AddRemoveRawsSbSizer7->Add( m_removeRawFilesButton, 0, wxALIGN_CENTER|wxALL, 5 );


	rawFilesSbSizer->Add( AddRemoveRawsSbSizer7, 0, wxEXPAND, 5 );


	mainPanelSizer->Add( rawFilesSbSizer, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* equivalentCliSbSizer;
	equivalentCliSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Equivalent CLI Command") ), wxVERTICAL );

	equivalentCliSbSizer->SetMinSize( wxSize( -1,150 ) );
	m_equivalentCliTextCtrl = new wxTextCtrl( equivalentCliSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	equivalentCliSbSizer->Add( m_equivalentCliTextCtrl, 1, wxALL|wxEXPAND, 5 );


	mainPanelSizer->Add( equivalentCliSbSizer, 0, wxALL|wxEXPAND, 5 );

	m_executeButton = new wxButton( m_inputPanel, wxID_ANY, _("Execute"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	mainPanelSizer->Add( m_executeButton, 0, wxALIGN_CENTER|wxALL, 10 );


	m_inputPanel->SetSizer( mainPanelSizer );
	m_inputPanel->Layout();
	mainPanelSizer->Fit( m_inputPanel );
	m_mainNotebook->AddPage( m_inputPanel, _("Input"), true );
	m_logPanel = new wxPanel( m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* logSizer;
	logSizer = new wxBoxSizer( wxVERTICAL );

	m_logOutputTextCtrl = new wxTextCtrl( m_logPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	logSizer->Add( m_logOutputTextCtrl, 1, wxALL|wxEXPAND, 5 );


	m_logPanel->SetSizer( logSizer );
	m_logPanel->Layout();
	logSizer->Fit( m_logPanel );
	m_mainNotebook->AddPage( m_logPanel, _("Log"), false );
	m_resultsPanel = new wxPanel( m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* resultsSizer;
	resultsSizer = new wxBoxSizer( wxVERTICAL );

	m_csvOutputStaticText = new wxStaticText( m_resultsPanel, wxID_ANY, _("CSV Output:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_csvOutputStaticText->Wrap( -1 );
	resultsSizer->Add( m_csvOutputStaticText, 0, wxALL, 5 );

	m_cvsGrid = new wxGrid( m_resultsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_cvsGrid->CreateGrid( 5, 5 );
	m_cvsGrid->EnableEditing( true );
	m_cvsGrid->EnableGridLines( true );
	m_cvsGrid->EnableDragGridSize( false );
	m_cvsGrid->SetMargins( 0, 0 );

	// Columns
	m_cvsGrid->EnableDragColMove( false );
	m_cvsGrid->EnableDragColSize( true );
	m_cvsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_cvsGrid->EnableDragRowSize( true );
	m_cvsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_cvsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	resultsSizer->Add( m_cvsGrid, 1, wxALL|wxEXPAND, 5 );

	m_generateGraphStaticText = new wxStaticText( m_resultsPanel, wxID_ANY, _("Generated Graph (placeholder):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_generateGraphStaticText->Wrap( -1 );
	resultsSizer->Add( m_generateGraphStaticText, 0, wxALL, 5 );

	m_processingGauge = new wxGauge( m_resultsPanel, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_processingGauge->SetValue( 0 );
	m_processingGauge->Hide();

	resultsSizer->Add( m_processingGauge, 0, wxALL|wxEXPAND, 5 );

	m_imageGraph = new wxStaticBitmap( m_resultsPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	resultsSizer->Add( m_imageGraph, 1, wxALL|wxEXPAND, 5 );


	m_resultsPanel->SetSizer( resultsSizer );
	m_resultsPanel->Layout();
	resultsSizer->Fit( m_resultsPanel );
	m_mainNotebook->AddPage( m_resultsPanel, _("Results"), false );

	mainSizer->Add( m_mainNotebook, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( mainSizer );
	this->Layout();

	this->Centre( wxBOTH );
}

MyFrameBase::~MyFrameBase()
{
}
