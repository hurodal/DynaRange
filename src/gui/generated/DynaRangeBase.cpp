///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "DynaRangeBase.h"

#include "../res/Icono_BAD_Mark_Red_alpha_22x22.png.h"

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

	wxBoxSizer* twoColMainPanelSizer;
	twoColMainPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* inputColLeftSizer;
	inputColLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* darkSatInputRawSizer;
	darkSatInputRawSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* darkSaturationSizer;
	darkSaturationSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* darkFrameSbSizer;
	darkFrameSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Dark Frame") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer1->AddGrowableRow( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_darkFileStaticText = new wxStaticText( darkFrameSbSizer->GetStaticBox(), wxID_ANY, _("Dark File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_darkFileStaticText->Wrap( -1 );
	fgSizer1->Add( m_darkFileStaticText, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

	m_darkFilePicker = new wxFilePickerCtrl( darkFrameSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select a file"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	fgSizer1->Add( m_darkFilePicker, 1, wxALL|wxEXPAND, 2 );

	m_clearDarkFileButton = new wxButton( darkFrameSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );

	m_clearDarkFileButton->SetBitmap( Icono_BAD_Mark_Red_alpha_22x22_png_to_wx_bitmap() );
	m_clearDarkFileButton->SetMaxSize( wxSize( 28,28 ) );

	fgSizer1->Add( m_clearDarkFileButton, 0, wxALL, 5 );

	m_darkValueStaticText = new wxStaticText( darkFrameSbSizer->GetStaticBox(), wxID_ANY, _("Dark Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_darkValueStaticText->Wrap( -1 );
	fgSizer1->Add( m_darkValueStaticText, 1, wxALL, 2 );

	m_darkValueTextCtrl = new wxTextCtrl( darkFrameSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_darkValueTextCtrl, 1, wxALL, 2 );


	darkFrameSbSizer->Add( fgSizer1, 0, wxALL|wxEXPAND, 3 );


	darkSaturationSizer->Add( darkFrameSbSizer, 0, wxALL, 5 );

	wxStaticBoxSizer* saturationSbSizer;
	saturationSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Saturation") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer2->AddGrowableRow( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_saturationFileStaticText = new wxStaticText( saturationSbSizer->GetStaticBox(), wxID_ANY, _("Sat. File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_saturationFileStaticText->Wrap( -1 );
	fgSizer2->Add( m_saturationFileStaticText, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

	m_saturationFilePicker = new wxFilePickerCtrl( saturationSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, _("Select a file"), _("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	fgSizer2->Add( m_saturationFilePicker, 1, wxALL|wxEXPAND, 2 );

	m_clearSaturationFileButton = new wxButton( saturationSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );

	m_clearSaturationFileButton->SetBitmap( Icono_BAD_Mark_Red_alpha_22x22_png_to_wx_bitmap() );
	m_clearSaturationFileButton->SetMaxSize( wxSize( 28,28 ) );

	fgSizer2->Add( m_clearSaturationFileButton, 0, wxALL, 5 );

	m_saturationValueStaticText = new wxStaticText( saturationSbSizer->GetStaticBox(), wxID_ANY, _("Sat. Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_saturationValueStaticText->Wrap( -1 );
	fgSizer2->Add( m_saturationValueStaticText, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

	m_saturationValueTextCtrl = new wxTextCtrl( saturationSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_saturationValueTextCtrl, 1, wxALL|wxEXPAND, 2 );


	saturationSbSizer->Add( fgSizer2, 1, wxALL|wxEXPAND, 3 );


	darkSaturationSizer->Add( saturationSbSizer, 0, wxALL, 5 );


	darkSatInputRawSizer->Add( darkSaturationSizer, 0, wxALL, 5 );

	wxBoxSizer* inputRawSizer;
	inputRawSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* rawFilesSbSizer;
	rawFilesSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Input RAW Files") ), wxVERTICAL );

	rawFilesSbSizer->SetMinSize( wxSize( -1,150 ) );
	m_rawFileslistBox = new wxListBox( rawFilesSbSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED );
	rawFilesSbSizer->Add( m_rawFileslistBox, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* AddRemoveRawsSbSizer7;
	AddRemoveRawsSbSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_addRawFilesButton = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Add RAW Files..."), wxDefaultPosition, wxDefaultSize, 0 );
	AddRemoveRawsSbSizer7->Add( m_addRawFilesButton, 0, wxALIGN_CENTER|wxALL, 5 );

	m_removeRawFilesButton = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Remove Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeRawFilesButton->Enable( false );

	AddRemoveRawsSbSizer7->Add( m_removeRawFilesButton, 0, wxALIGN_CENTER|wxALL, 5 );

	m_removeAllFiles = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Remove All"), wxDefaultPosition, wxDefaultSize, wxBU_LEFT );

	m_removeAllFiles->SetBitmap( Icono_BAD_Mark_Red_alpha_22x22_png_to_wx_bitmap() );
	AddRemoveRawsSbSizer7->Add( m_removeAllFiles, 0, wxALL|wxEXPAND, 5 );


	rawFilesSbSizer->Add( AddRemoveRawsSbSizer7, 0, wxEXPAND, 5 );


	inputRawSizer->Add( rawFilesSbSizer, 1, wxALL|wxEXPAND, 5 );


	darkSatInputRawSizer->Add( inputRawSizer, 1, wxALL|wxEXPAND, 5 );


	inputColLeftSizer->Add( darkSatInputRawSizer, 0, wxEXPAND, 5 );

	m_rawImagePreviewPanel = new wxPanel( m_inputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	inputColLeftSizer->Add( m_rawImagePreviewPanel, 1, wxEXPAND | wxALL, 5 );


	twoColMainPanelSizer->Add( inputColLeftSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* inputColRightSizer;
	inputColRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* analysisGraphSizer;
	analysisGraphSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* analysisParamsSizer;
	analysisParamsSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Analysis Parameters") ), wxVERTICAL );

	wxBoxSizer* patchRatioSizer;
	patchRatioSizer = new wxBoxSizer( wxHORIZONTAL );

	m_patchRatioStaticText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("Patch Ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_patchRatioStaticText->Wrap( -1 );
	patchRatioSizer->Add( m_patchRatioStaticText, 0, wxALL|wxEXPAND, 5 );

	m_patchRatioSlider = new wxSlider( analysisParamsSizer->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	patchRatioSizer->Add( m_patchRatioSlider, 1, wxALL|wxEXPAND, 5 );

	m_patchRatioValueText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("0.50"), wxDefaultPosition, wxDefaultSize, 0 );
	m_patchRatioValueText->Wrap( -1 );
	m_patchRatioValueText->SetMinSize( wxSize( 40,-1 ) );

	patchRatioSizer->Add( m_patchRatioValueText, 0, wxALL|wxEXPAND, 5 );


	analysisParamsSizer->Add( patchRatioSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* drNormalizationSizer;
	drNormalizationSizer = new wxBoxSizer( wxHORIZONTAL );

	m_drNormalizationStaticText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("DR Normalization"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drNormalizationStaticText->Wrap( -1 );
	drNormalizationSizer->Add( m_drNormalizationStaticText, 0, wxALL|wxEXPAND, 5 );

	m_drNormalizationSlider = new wxSlider( analysisParamsSizer->GetStaticBox(), wxID_ANY, 0, 0, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	drNormalizationSizer->Add( m_drNormalizationSlider, 1, wxALL|wxEXPAND, 5 );

	m_drNormalizationValueText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("0Mpx"), wxDefaultPosition, wxDefaultSize, 0 );
	m_drNormalizationValueText->Wrap( -1 );
	drNormalizationSizer->Add( m_drNormalizationValueText, 0, wxALL|wxEXPAND, 5 );


	analysisParamsSizer->Add( drNormalizationSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* snrThresholdSizer;
	snrThresholdSizer = new wxBoxSizer( wxHORIZONTAL );

	m_snrThresholdStaticText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("SNR Thresholds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snrThresholdStaticText->Wrap( -1 );
	snrThresholdSizer->Add( m_snrThresholdStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_snrThresholdsValues = new wxTextCtrl( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("0 12"), wxDefaultPosition, wxDefaultSize, 0 );
	snrThresholdSizer->Add( m_snrThresholdsValues, 1, wxALL, 5 );


	analysisParamsSizer->Add( snrThresholdSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* polyOutLogSizer;
	polyOutLogSizer = new wxBoxSizer( wxHORIZONTAL );

	m_polynomicStaticText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("Polynomic Order"), wxDefaultPosition, wxDefaultSize, 0 );
	m_polynomicStaticText->Wrap( -1 );
	polyOutLogSizer->Add( m_polynomicStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	wxString m_PlotChoiceChoices[] = { _("2"), _("3") };
	int m_PlotChoiceNChoices = sizeof( m_PlotChoiceChoices ) / sizeof( wxString );
	m_PlotChoice = new wxChoice( analysisParamsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PlotChoiceNChoices, m_PlotChoiceChoices, 0 );
	m_PlotChoice->SetSelection( 1 );
	polyOutLogSizer->Add( m_PlotChoice, 0, wxALL, 5 );

	m_outputStaticText = new wxStaticText( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("Ouput file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputStaticText->Wrap( -1 );
	polyOutLogSizer->Add( m_outputStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_outputTextCtrl = new wxTextCtrl( analysisParamsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	polyOutLogSizer->Add( m_outputTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_saveLog = new wxCheckBox( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("Save Log"), wxDefaultPosition, wxDefaultSize, 0 );
	polyOutLogSizer->Add( m_saveLog, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	analysisParamsSizer->Add( polyOutLogSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* channelsSizer;
	channelsSizer = new wxStaticBoxSizer( new wxStaticBox( analysisParamsSizer->GetStaticBox(), wxID_ANY, _("Channels") ), wxHORIZONTAL );

	R_staticText = new wxStaticText( channelsSizer->GetStaticBox(), wxID_ANY, _("R"), wxDefaultPosition, wxDefaultSize, 0 );
	R_staticText->Wrap( -1 );
	channelsSizer->Add( R_staticText, 0, wxALL, 5 );

	R_checkBox = new wxCheckBox( channelsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	channelsSizer->Add( R_checkBox, 0, wxALL, 5 );

	G1_staticText = new wxStaticText( channelsSizer->GetStaticBox(), wxID_ANY, _("G1"), wxDefaultPosition, wxDefaultSize, 0 );
	G1_staticText->Wrap( -1 );
	channelsSizer->Add( G1_staticText, 0, wxALL, 5 );

	G1_checkBox = new wxCheckBox( channelsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	channelsSizer->Add( G1_checkBox, 0, wxALL, 5 );

	G2_staticText = new wxStaticText( channelsSizer->GetStaticBox(), wxID_ANY, _("G2"), wxDefaultPosition, wxDefaultSize, 0 );
	G2_staticText->Wrap( -1 );
	channelsSizer->Add( G2_staticText, 0, wxALL, 5 );

	G2_checkBox = new wxCheckBox( channelsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	G2_checkBox->SetValue(true);
	channelsSizer->Add( G2_checkBox, 0, wxALL, 5 );

	B_staticText = new wxStaticText( channelsSizer->GetStaticBox(), wxID_ANY, _("B"), wxDefaultPosition, wxDefaultSize, 0 );
	B_staticText->Wrap( -1 );
	channelsSizer->Add( B_staticText, 0, wxALL, 5 );

	B_checkBox = new wxCheckBox( channelsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	channelsSizer->Add( B_checkBox, 0, wxALL, 5 );


	channelsSizer->Add( 15, 0, 0, wxEXPAND, 5 );

	AVG_staticText = new wxStaticText( channelsSizer->GetStaticBox(), wxID_ANY, _("Average"), wxDefaultPosition, wxDefaultSize, 0 );
	AVG_staticText->Wrap( -1 );
	channelsSizer->Add( AVG_staticText, 0, wxALL, 5 );

	wxString AVG_ChoiceValueChoices[] = { _("No"), _("Full"), _("Only Selected") };
	int AVG_ChoiceValueNChoices = sizeof( AVG_ChoiceValueChoices ) / sizeof( wxString );
	AVG_ChoiceValue = new wxChoice( channelsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, AVG_ChoiceValueNChoices, AVG_ChoiceValueChoices, 0 );
	AVG_ChoiceValue->SetSelection( 0 );
	channelsSizer->Add( AVG_ChoiceValue, 0, 0, 5 );


	analysisParamsSizer->Add( channelsSizer, 0, wxALL|wxEXPAND, 5 );


	analysisGraphSizer->Add( analysisParamsSizer, 0, wxALL|wxEXPAND, 5 );


	inputColRightSizer->Add( analysisGraphSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer50;
	bSizer50 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer491;
	bSizer491 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* chartCoordStaticSizer;
	chartCoordStaticSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Chart Coordinates") ), wxVERTICAL );

	wxFlexGridSizer* coordFlexSizer;
	coordFlexSizer = new wxFlexGridSizer( 0, 4, 0, 0 );
	coordFlexSizer->SetFlexibleDirection( wxBOTH );
	coordFlexSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText38 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("X1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	coordFlexSizer->Add( m_staticText38, 0, wxALL, 5 );

	m_coordX1Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordX1Value, 0, wxALL, 3 );

	m_staticText381 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("Y1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText381->Wrap( -1 );
	coordFlexSizer->Add( m_staticText381, 0, wxALL, 5 );

	m_coordY1Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordY1Value, 0, wxALL, 3 );

	m_staticText42 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("X2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText42->Wrap( -1 );
	coordFlexSizer->Add( m_staticText42, 0, wxALL, 5 );

	m_coordX2Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordX2Value, 0, wxALL, 3 );

	m_staticText43 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("Y2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText43->Wrap( -1 );
	coordFlexSizer->Add( m_staticText43, 0, wxALL, 5 );

	m_coordY2Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordY2Value, 0, wxALL, 3 );

	m_staticText44 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("X3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	coordFlexSizer->Add( m_staticText44, 0, wxALL, 5 );

	m_coordX3Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordX3Value, 0, wxALL, 3 );

	m_staticText45 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("Y3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	coordFlexSizer->Add( m_staticText45, 0, wxALL, 5 );

	m_coordY3Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordY3Value, 0, wxALL, 3 );

	m_staticText46 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("X4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText46->Wrap( -1 );
	coordFlexSizer->Add( m_staticText46, 0, wxALL, 5 );

	m_coordX4Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordX4Value, 0, wxALL, 3 );

	m_staticText47 = new wxStaticText( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("Y4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText47->Wrap( -1 );
	coordFlexSizer->Add( m_staticText47, 0, wxALL, 5 );

	m_coordY4Value = new wxTextCtrl( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	coordFlexSizer->Add( m_coordY4Value, 0, wxALL, 3 );


	chartCoordStaticSizer->Add( coordFlexSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* buttChartCoordSizer;
	buttChartCoordSizer = new wxBoxSizer( wxHORIZONTAL );

	m_clearAllCoordinates = new wxButton( chartCoordStaticSizer->GetStaticBox(), wxID_ANY, _("Clear All"), wxDefaultPosition, wxDefaultSize, 0 );

	m_clearAllCoordinates->SetBitmap( Icono_BAD_Mark_Red_alpha_22x22_png_to_wx_bitmap() );
	buttChartCoordSizer->Add( m_clearAllCoordinates, 0, wxALL, 5 );


	chartCoordStaticSizer->Add( buttChartCoordSizer, 1, wxEXPAND, 5 );


	bSizer491->Add( chartCoordStaticSizer, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* patchesStaticSizer;
	patchesStaticSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Pacthes") ), wxVERTICAL );

	wxStaticBoxSizer* chatPatchStaticSizer;
	chatPatchStaticSizer = new wxStaticBoxSizer( new wxStaticBox( patchesStaticSizer->GetStaticBox(), wxID_ANY, _("Chart patches") ), wxVERTICAL );

	wxBoxSizer* chartPatchSizer1;
	chartPatchSizer1 = new wxBoxSizer( wxVERTICAL );


	chatPatchStaticSizer->Add( chartPatchSizer1, 0, wxALL, 5 );

	wxFlexGridSizer* chartPatchFgSizer;
	chartPatchFgSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	chartPatchFgSizer->SetFlexibleDirection( wxBOTH );
	chartPatchFgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	chartPatchRowStaticText1 = new wxStaticText( chatPatchStaticSizer->GetStaticBox(), wxID_ANY, _("Rows."), wxDefaultPosition, wxDefaultSize, 0 );
	chartPatchRowStaticText1->Wrap( -1 );
	chartPatchFgSizer->Add( chartPatchRowStaticText1, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartPatchRowValue1 = new wxTextCtrl( chatPatchStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	chartPatchFgSizer->Add( m_chartPatchRowValue1, 0, wxALIGN_CENTER|wxALL, 5 );

	chartPatchColStaticText1 = new wxStaticText( chatPatchStaticSizer->GetStaticBox(), wxID_ANY, _("Cols."), wxDefaultPosition, wxDefaultSize, 0 );
	chartPatchColStaticText1->Wrap( -1 );
	chartPatchFgSizer->Add( chartPatchColStaticText1, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartPatchColValue1 = new wxTextCtrl( chatPatchStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 70,-1 ), 0 );
	chartPatchFgSizer->Add( m_chartPatchColValue1, 0, wxALIGN_CENTER|wxALL, 5 );


	chatPatchStaticSizer->Add( chartPatchFgSizer, 1, wxEXPAND, 5 );


	patchesStaticSizer->Add( chatPatchStaticSizer, 0, wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* debugPatchesStaticSizer;
	debugPatchesStaticSizer = new wxStaticBoxSizer( new wxStaticBox( patchesStaticSizer->GetStaticBox(), wxID_ANY, _("Debug patches") ), wxHORIZONTAL );

	wxBoxSizer* m_debugPatchesSizer;
	m_debugPatchesSizer = new wxBoxSizer( wxHORIZONTAL );

	m_debugPatchesCheckBox = new wxCheckBox( debugPatchesStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_debugPatchesSizer->Add( m_debugPatchesCheckBox, 0, wxALIGN_CENTER|wxALL, 5 );

	m_debugPatchesFileNameValue = new wxTextCtrl( debugPatchesStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_debugPatchesFileNameValue->Enable( false );

	m_debugPatchesSizer->Add( m_debugPatchesFileNameValue, 1, wxALL|wxEXPAND, 5 );


	debugPatchesStaticSizer->Add( m_debugPatchesSizer, 1, wxEXPAND, 5 );


	patchesStaticSizer->Add( debugPatchesStaticSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer491->Add( patchesStaticSizer, 1, wxEXPAND|wxLEFT, 5 );


	bSizer50->Add( bSizer491, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* graphicSizer;
	graphicSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Graphics") ), wxVERTICAL );

	wxBoxSizer* plotParamsSizer;
	plotParamsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_plotParamScattersCheckBox = new wxCheckBox( graphicSizer->GetStaticBox(), wxID_ANY, _("Scatters"), wxDefaultPosition, wxDefaultSize, 0 );
	plotParamsSizer->Add( m_plotParamScattersCheckBox, 0, 0, 5 );

	m_plotParamCurveCheckBox = new wxCheckBox( graphicSizer->GetStaticBox(), wxID_ANY, _("Curves"), wxDefaultPosition, wxDefaultSize, 0 );
	plotParamsSizer->Add( m_plotParamCurveCheckBox, 0, 0, 5 );

	m_plotParamLabelsCheckBox = new wxCheckBox( graphicSizer->GetStaticBox(), wxID_ANY, _("Labels"), wxDefaultPosition, wxDefaultSize, 0 );
	plotParamsSizer->Add( m_plotParamLabelsCheckBox, 0, 0, 5 );


	plotParamsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	allIsosCheckBox = new wxCheckBox( graphicSizer->GetStaticBox(), wxID_ANY, _("All ISOs"), wxDefaultPosition, wxDefaultSize, 0 );
	plotParamsSizer->Add( allIsosCheckBox, 0, 0, 5 );


	graphicSizer->Add( plotParamsSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* graphicsOthersSizer;
	graphicsOthersSizer = new wxBoxSizer( wxHORIZONTAL );

	graphicFormatStaticText = new wxStaticText( graphicSizer->GetStaticBox(), wxID_ANY, _("Graphic format"), wxDefaultPosition, wxDefaultSize, 0 );
	graphicFormatStaticText->Wrap( -1 );
	graphicsOthersSizer->Add( graphicFormatStaticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_plotFormatChoiceChoices[] = { _("PNG"), _("PDF"), _("SVG") };
	int m_plotFormatChoiceNChoices = sizeof( m_plotFormatChoiceChoices ) / sizeof( wxString );
	m_plotFormatChoice = new wxChoice( graphicSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotFormatChoiceNChoices, m_plotFormatChoiceChoices, 0 );
	m_plotFormatChoice->SetSelection( 0 );
	graphicsOthersSizer->Add( m_plotFormatChoice, 0, 0, 5 );

	m_plotingStaticText = new wxStaticText( graphicSizer->GetStaticBox(), wxID_ANY, _("Graphic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotingStaticText->Wrap( -1 );
	graphicsOthersSizer->Add( m_plotingStaticText, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

	wxString m_plotingChoiceChoices[] = { _("No"), _("Graphic"), _("Graphic + Short Command"), _("Graphic + Long Command") };
	int m_plotingChoiceNChoices = sizeof( m_plotingChoiceChoices ) / sizeof( wxString );
	m_plotingChoice = new wxChoice( graphicSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotingChoiceNChoices, m_plotingChoiceChoices, 0 );
	m_plotingChoice->SetSelection( 3 );
	graphicsOthersSizer->Add( m_plotingChoice, 0, wxALL, 5 );


	graphicSizer->Add( graphicsOthersSizer, 1, wxEXPAND, 5 );


	bSizer44->Add( graphicSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer50->Add( bSizer44, 0, wxEXPAND, 5 );


	inputColRightSizer->Add( bSizer50, 0, 0, 5 );


	twoColMainPanelSizer->Add( inputColRightSizer, 0, wxEXPAND, 5 );


	mainPanelSizer->Add( twoColMainPanelSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* footSizer;
	footSizer = new wxBoxSizer( wxVERTICAL );

	m_executeButton = new wxButton( m_inputPanel, wxID_ANY, _("Execute"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	footSizer->Add( m_executeButton, 0, wxALIGN_CENTER|wxALL, 10 );


	mainPanelSizer->Add( footSizer, 0, wxEXPAND, 5 );


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

	m_splitterResults = new wxSplitterWindow( m_resultsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterResults->SetSashGravity( 0.3 );
	m_splitterResults->Connect( wxEVT_IDLE, wxIdleEventHandler( MyFrameBase::m_splitterResultsOnIdle ), NULL, this );
	m_splitterResults->SetMinimumPaneSize( 50 );

	m_leftPanel = new wxPanel( m_splitterResults, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_csvOutputStaticText = new wxStaticText( m_leftPanel, wxID_ANY, _("CSV Output:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_csvOutputStaticText->Wrap( -1 );
	bSizer16->Add( m_csvOutputStaticText, 0, wxALL, 5 );

	m_cvsGrid = new wxGrid( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

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
	bSizer16->Add( m_cvsGrid, 1, wxALL|wxEXPAND, 5 );


	m_leftPanel->SetSizer( bSizer16 );
	m_leftPanel->Layout();
	bSizer16->Fit( m_leftPanel );
	m_rightPanel = new wxPanel( m_splitterResults, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );

	m_generateGraphStaticText = new wxStaticText( m_rightPanel, wxID_ANY, _("Generated Graph (placeholder):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_generateGraphStaticText->Wrap( -1 );
	bSizer18->Add( m_generateGraphStaticText, 0, wxALL, 5 );

	m_processingGauge = new wxGauge( m_rightPanel, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_processingGauge->SetValue( 0 );
	m_processingGauge->Hide();

	bSizer18->Add( m_processingGauge, 0, wxALL|wxEXPAND, 5 );

	m_webViewPlaceholderPanel = new wxPanel( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizer18->Add( m_webViewPlaceholderPanel, 1, wxEXPAND | wxALL, 5 );


	m_rightPanel->SetSizer( bSizer18 );
	m_rightPanel->Layout();
	bSizer18->Fit( m_rightPanel );
	m_splitterResults->SplitVertically( m_leftPanel, m_rightPanel, 0 );
	resultsSizer->Add( m_splitterResults, 1, wxEXPAND, 5 );


	m_resultsPanel->SetSizer( resultsSizer );
	m_resultsPanel->Layout();
	resultsSizer->Fit( m_resultsPanel );
	m_mainNotebook->AddPage( m_resultsPanel, _("Results"), false );
	m_chartPanel = new wxPanel( m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_splitterChart = new wxSplitterWindow( m_chartPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterChart->Connect( wxEVT_IDLE, wxIdleEventHandler( MyFrameBase::m_splitterChartOnIdle ), NULL, this );

	leftColChartPanel = new wxPanel( m_splitterChart, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* chartColorStaticSizer;
	chartColorStaticSizer = new wxStaticBoxSizer( new wxStaticBox( leftColChartPanel, wxID_ANY, _("Chart colors") ), wxVERTICAL );

	wxBoxSizer* rParamSizer;
	rParamSizer = new wxBoxSizer( wxHORIZONTAL );

	m_rParamStaticText = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("Red"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rParamStaticText->Wrap( -1 );
	m_rParamStaticText->SetMinSize( wxSize( 40,-1 ) );

	rParamSizer->Add( m_rParamStaticText, 0, wxALL, 5 );

	m_rParamSlider = new wxSlider( chartColorStaticSizer->GetStaticBox(), wxID_ANY, 0, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	rParamSizer->Add( m_rParamSlider, 1, wxALL, 5 );

	m_rParamValue = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("255"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rParamValue->Wrap( -1 );
	rParamSizer->Add( m_rParamValue, 0, wxALL, 5 );


	chartColorStaticSizer->Add( rParamSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* gParamSizer;
	gParamSizer = new wxBoxSizer( wxHORIZONTAL );

	m_gParamStaticText = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("Green"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gParamStaticText->Wrap( -1 );
	m_gParamStaticText->SetMinSize( wxSize( 40,-1 ) );

	gParamSizer->Add( m_gParamStaticText, 0, wxALL, 5 );

	m_gParamSlider = new wxSlider( chartColorStaticSizer->GetStaticBox(), wxID_ANY, 0, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	gParamSizer->Add( m_gParamSlider, 1, wxALL, 5 );

	m_gParamValue = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("255"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gParamValue->Wrap( -1 );
	gParamSizer->Add( m_gParamValue, 0, wxALL, 5 );


	chartColorStaticSizer->Add( gParamSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* bParamSizer;
	bParamSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bParamStaticText = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("Blue"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bParamStaticText->Wrap( -1 );
	m_bParamStaticText->SetMinSize( wxSize( 40,-1 ) );

	bParamSizer->Add( m_bParamStaticText, 0, wxALL, 5 );

	m_bParamSlider = new wxSlider( chartColorStaticSizer->GetStaticBox(), wxID_ANY, 0, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bParamSizer->Add( m_bParamSlider, 1, wxALL, 5 );

	m_bParamValue = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("255"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bParamValue->Wrap( -1 );
	bParamSizer->Add( m_bParamValue, 0, wxALL, 5 );


	chartColorStaticSizer->Add( bParamSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* chartInvGammaSizer;
	chartInvGammaSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticText22 = new wxStaticText( chartColorStaticSizer->GetStaticBox(), wxID_ANY, _("InvGamma"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	chartInvGammaSizer->Add( m_staticText22, 0, wxALIGN_CENTER|wxALL, 5 );

	m_InvGammaValue = new wxTextCtrl( chartColorStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartInvGammaSizer->Add( m_InvGammaValue, 0, wxALL, 5 );


	chartColorStaticSizer->Add( chartInvGammaSizer, 1, wxEXPAND, 5 );


	bSizer29->Add( chartColorStaticSizer, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* chartDimStaticSizer;
	chartDimStaticSizer = new wxStaticBoxSizer( new wxStaticBox( leftColChartPanel, wxID_ANY, _("Chart dimensions") ), wxVERTICAL );

	wxBoxSizer* chartDimXSizer;
	chartDimXSizer = new wxBoxSizer( wxHORIZONTAL );

	m_chartDimXStaticText = new wxStaticText( chartDimStaticSizer->GetStaticBox(), wxID_ANY, _("DimX"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chartDimXStaticText->Wrap( -1 );
	chartDimXSizer->Add( m_chartDimXStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartDimXValue = new wxTextCtrl( chartDimStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartDimXSizer->Add( m_chartDimXValue, 0, wxALL, 5 );


	chartDimStaticSizer->Add( chartDimXSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* chartDimWHSizer;
	chartDimWHSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* chartDimWSizer;
	chartDimWSizer = new wxBoxSizer( wxHORIZONTAL );

	m_chartDimWStaticText = new wxStaticText( chartDimStaticSizer->GetStaticBox(), wxID_ANY, _("Width proportion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chartDimWStaticText->Wrap( -1 );
	chartDimWSizer->Add( m_chartDimWStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartDimWValue = new wxTextCtrl( chartDimStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartDimWSizer->Add( m_chartDimWValue, 0, wxALIGN_CENTER|wxALL, 5 );


	chartDimWHSizer->Add( chartDimWSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* chartDimHSizer;
	chartDimHSizer = new wxBoxSizer( wxHORIZONTAL );

	m_chartDimHStaticText = new wxStaticText( chartDimStaticSizer->GetStaticBox(), wxID_ANY, _("Height proportion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chartDimHStaticText->Wrap( -1 );
	chartDimHSizer->Add( m_chartDimHStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartDimHValue = new wxTextCtrl( chartDimStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartDimHSizer->Add( m_chartDimHValue, 0, wxALIGN_CENTER|wxALL, 5 );


	chartDimWHSizer->Add( chartDimHSizer, 1, wxEXPAND, 5 );


	chartDimStaticSizer->Add( chartDimWHSizer, 0, 0, 5 );


	bSizer29->Add( chartDimStaticSizer, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* chartPatchStaticSizer;
	chartPatchStaticSizer = new wxStaticBoxSizer( new wxStaticBox( leftColChartPanel, wxID_ANY, _("Chart patches") ), wxVERTICAL );

	wxBoxSizer* chartPatchSizer;
	chartPatchSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* chartPatchRowSizer;
	chartPatchRowSizer = new wxBoxSizer( wxHORIZONTAL );

	m_chartPatchRowStaticText = new wxStaticText( chartPatchStaticSizer->GetStaticBox(), wxID_ANY, _("Rows"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chartPatchRowStaticText->Wrap( -1 );
	chartPatchRowSizer->Add( m_chartPatchRowStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartPatchRowValue = new wxTextCtrl( chartPatchStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartPatchRowSizer->Add( m_chartPatchRowValue, 0, wxALIGN_CENTER|wxALL, 5 );


	chartPatchSizer->Add( chartPatchRowSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* chartPatchColSizer;
	chartPatchColSizer = new wxBoxSizer( wxHORIZONTAL );

	chartPatchColStaticText = new wxStaticText( chartPatchStaticSizer->GetStaticBox(), wxID_ANY, _("Colums"), wxDefaultPosition, wxDefaultSize, 0 );
	chartPatchColStaticText->Wrap( -1 );
	chartPatchColSizer->Add( chartPatchColStaticText, 0, wxALIGN_CENTER|wxALL, 5 );

	m_chartPatchColValue = new wxTextCtrl( chartPatchStaticSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	chartPatchColSizer->Add( m_chartPatchColValue, 0, wxALIGN_CENTER|wxALL, 5 );


	chartPatchSizer->Add( chartPatchColSizer, 1, wxEXPAND, 5 );


	chartPatchStaticSizer->Add( chartPatchSizer, 0, 0, 5 );


	bSizer29->Add( chartPatchStaticSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* chartButtonsSizer;
	chartButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	chartButtonCreate = new wxButton( leftColChartPanel, wxID_ANY, _("Create chart"), wxDefaultPosition, wxDefaultSize, 0 );
	chartButtonsSizer->Add( chartButtonCreate, 0, wxALL, 5 );


	chartButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	chartButtonPreview = new wxButton( leftColChartPanel, wxID_ANY, _("Preview chart"), wxDefaultPosition, wxDefaultSize, 0 );
	chartButtonsSizer->Add( chartButtonPreview, 0, wxALL, 5 );


	bSizer29->Add( chartButtonsSizer, 0, wxALL|wxEXPAND, 5 );


	leftColChartPanel->SetSizer( bSizer29 );
	leftColChartPanel->Layout();
	bSizer29->Fit( leftColChartPanel );
	rightColChartPanel = new wxPanel( m_splitterChart, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* chartPreviewImageSizer;
	chartPreviewImageSizer = new wxBoxSizer( wxVERTICAL );

	m_webView2PlaceholderPanel = new wxPanel( rightColChartPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	chartPreviewImageSizer->Add( m_webView2PlaceholderPanel, 1, wxEXPAND | wxALL, 5 );


	rightColChartPanel->SetSizer( chartPreviewImageSizer );
	rightColChartPanel->Layout();
	chartPreviewImageSizer->Fit( rightColChartPanel );
	m_splitterChart->SplitVertically( leftColChartPanel, rightColChartPanel, 450 );
	bSizer21->Add( m_splitterChart, 1, wxEXPAND, 5 );


	m_chartPanel->SetSizer( bSizer21 );
	m_chartPanel->Layout();
	bSizer21->Fit( m_chartPanel );
	m_mainNotebook->AddPage( m_chartPanel, _("Chart"), false );
	m_equCliPanel = new wxPanel( m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* equivalentSbSizer;
	equivalentSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_equCliPanel, wxID_ANY, _("Equivalent CLI command") ), wxVERTICAL );

	equivalentSbSizer->SetMinSize( wxSize( -1,100 ) );
	m_equivalentCliTextCtrl = new wxTextCtrl( equivalentSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_equivalentCliTextCtrl->SetMinSize( wxSize( -1,100 ) );

	equivalentSbSizer->Add( m_equivalentCliTextCtrl, 1, wxALL|wxEXPAND, 5 );


	m_equCliPanel->SetSizer( equivalentSbSizer );
	m_equCliPanel->Layout();
	equivalentSbSizer->Fit( m_equCliPanel );
	m_mainNotebook->AddPage( m_equCliPanel, _("Equ.CLI Command"), false );

	mainSizer->Add( m_mainNotebook, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

MyFrameBase::~MyFrameBase()
{
}
