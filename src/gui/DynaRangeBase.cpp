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

	wxStaticBoxSizer* rawFilesSbSizer;
	rawFilesSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Input RAW Files") ), wxVERTICAL );

	m_rawFileslistBox = new wxListBox( rawFilesSbSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	rawFilesSbSizer->Add( m_rawFileslistBox, 1, wxALL|wxEXPAND, 5 );

	m_addRawFilesButton = new wxButton( rawFilesSbSizer->GetStaticBox(), wxID_ANY, _("Add RAW Files..."), wxDefaultPosition, wxDefaultSize, 0 );
	rawFilesSbSizer->Add( m_addRawFilesButton, 0, wxALIGN_CENTER|wxALL, 5 );


	mainPanelSizer->Add( rawFilesSbSizer, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* equivalentCliSbSizer;
	equivalentCliSbSizer = new wxStaticBoxSizer( new wxStaticBox( m_inputPanel, wxID_ANY, _("Equivalent CLI Command") ), wxVERTICAL );

	m_equivalentCliTextCtrl = new wxTextCtrl( equivalentCliSbSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	equivalentCliSbSizer->Add( m_equivalentCliTextCtrl, 0, wxALL|wxEXPAND, 5 );


	mainPanelSizer->Add( equivalentCliSbSizer, 0, wxALL|wxEXPAND, 5 );

	m_executeButton = new wxButton( m_inputPanel, wxID_ANY, _("Execute"), wxDefaultPosition, wxDefaultSize, 0 );
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
