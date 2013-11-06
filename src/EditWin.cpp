#include "EditWin.h"

#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <VolumeRoster.h>
#include <Volume.h>

#include "App.h"
#include "Preference.h"

EditWin::EditWin()
	: BWindow(BRect(200, 200, 462, 356), 
		"Edit Query", 
		B_TITLED_WINDOW, 
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	Init();
}

inline void
EditWin::Init()
{
	//background
	BView* bg = new BView( Bounds(), "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bg);
	
	//text controls
	fName = new BTextControl( 
		BRect(12, 10, 250, 29), 
		"nameTC", 
		"Name:", 
		NULL, 
		new BMessage(EDITWIN_NAMECHANGE) );
	fName->SetDivider(40.0);
	bg->AddChild(fName);
	
	fQuery = new BTextControl( BRect(12, 37, 250, 56), "queryTC", "Query:", NULL, new BMessage(EDITWIN_QUERYCHANGE) );
	fQuery->SetDivider(40.0);
	bg->AddChild(fQuery);
	
	//volume popup
	BPopUpMenu* volumeMenu = new BPopUpMenu("--select--", true, true);
	BMenuField* mf = new BMenuField(BRect(12, 64, 249, 87), "volumeMF", "Volumes:", volumeMenu);
	bg->AddChild(mf);

	//add "all" volume
	BMessage* allVolsMsg = new BMessage(EDITWIN_ALLVOLS_SEL);
	allVolsMsg->AddInt32("device", ALLVOLS);
	BMenuItem* allItem = new BMenuItem("All Volumes", allVolsMsg);
	allItem->SetMarked(true);
	volumeMenu->AddItem(allItem);
	
	//add volumes via BVolumeRoster
	fCurDevice = -1;
	BVolumeRoster volRos;
	BVolume vol;
	char volName[B_FILE_NAME_LENGTH];
	while( volRos.GetNextVolume(&vol) == B_OK )
	{
		if( vol.KnowsQuery() )
		{
			vol.GetName(volName);
			BMessage* miMsg = new BMessage(EDITWIN_VOLSELECT);
			miMsg->AddInt32("device", vol.Device() );
			BMenuItem* mi = new BMenuItem(volName, miMsg);
			volumeMenu->AddItem( mi );
		}
	}
	
	//show results checkbox
	fCurShowCount = false;
	fShowResults = new BCheckBox( BRect(12, 97, 249, 115), "ShowResults", "Show File Count", new BMessage(RESULTS_CB_CHANGE) );
	bg->AddChild(fShowResults);

	//buttons
	BButton* okbutton = new BButton( BRect(134, 125, 184, 148), "OkBtn", "Ok", new BMessage(EDITWIN_OK), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT );
	okbutton->MakeDefault(true);
	bg->AddChild(okbutton);
	
	BButton* delbutton = new BButton( BRect(199, 125, 249, 148), "CancelBtn", "Cancel", new BMessage(EDITWIN_CANCEL), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT );
	bg->AddChild(delbutton);
}

EditWin::~EditWin()
{
}

void
EditWin::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case EDITWIN_OK:
		{
			Preference pref(APP_SIG);		
			pref.Load();
			
			BMessage queryMsg(B_OK);
			queryMsg.AddString("query", fQuery->Text());
			queryMsg.AddString("name", fName->Text());
			queryMsg.AddInt32("showCount", (int32)fCurShowCount);
			queryMsg.AddInt32("volume", fCurDevice);
			
			pref.AddMessage("query", &queryMsg);
			pref.Save();
			
			be_app->PostMessage(EDITWIN_OK);
			this->PostMessage(B_QUIT_REQUESTED);
		}	
			break;
		case EDITWIN_CANCEL:
			this->PostMessage(B_QUIT_REQUESTED);
			break;
			
		case RESULTS_CB_CHANGE:
			fCurShowCount = ! fCurShowCount;
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}
