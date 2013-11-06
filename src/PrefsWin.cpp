#include "PrefsWin.h"

#include <Button.h>
#include <String.h>
#include <View.h>
#include <Volume.h>

#include <iostream>
using namespace std;

#include "App.h"
#include "EditWin.h"

#include "CLVEasyItem.h"
#include "Preference.h"

PrefsWin::PrefsWin()
	: BWindow(BRect(100, 100, 549, 398), "Queries", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BView* bg = new BView( Bounds(), "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bg);
	
	CLVContainerView* clvc;
	fCLV = new ColumnListView(
		BRect(14, 9, 421, 245),
		&clvc,
		"clv",
		B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS,
		B_SINGLE_SELECTION_LIST,
		false,
		true,
		true,
		true,
		B_FANCY_BORDER,
		be_plain_font );
	fCLV->SetInvocationMessage( new BMessage(QUERY_LIST_INV) );
	bg->AddChild(clvc);
	
	CLVColumn* col;
	//col 1: name and (below) querystring
	col = new CLVColumn("Query", 155, CLV_NOT_MOVABLE | CLV_TELL_ITEMS_WIDTH, 100);
	fCLV->AddColumn(col);

	col = new CLVColumn("Volumes", 170, CLV_NOT_MOVABLE | CLV_NOT_RESIZABLE, 80);
	fCLV->AddColumn(col);

	col = new CLVColumn("Show Matches", 80, CLV_NOT_MOVABLE | CLV_NOT_RESIZABLE, 80);
	fCLV->AddColumn(col);

	BButton* addbutton = new BButton( BRect(14, 267, 66, 291), "AddBtn", "Add", new BMessage(ADD_QUERY), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT );
	BButton* delbutton = new BButton( BRect(81, 267, 137, 291), "DelBtn", "Delete", new BMessage(DEL_QUERY), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT );
	
	bg->AddChild(addbutton);
	bg->AddChild(delbutton);
	
	// load existing queries
	Preference pref(APP_SIG);
	pref.Load();
	
	int32 ct;
	type_code t;
	if( pref.GetInfo("query", &t, &ct) == B_OK)
	{
		const char* name, *query;
		int32 showCount;
		dev_t volumes;
		for( int32 ctr=0; ctr< ct; ctr++)
		{
			pref.FindString("name", &name);
			pref.FindString("query", &query);
			pref.FindInt32("showCount", &showCount);
			pref.FindInt32("volume", &volumes);
			AddQuery( name, query, showCount, volumes);
		}
	}
}

PrefsWin::~PrefsWin()
{
	BMessage winCtMsg(B_COUNT_PROPERTIES);
	winCtMsg.AddSpecifier("Window");
	
	BMessage reply;
	be_app_messenger.SendMessage(&winCtMsg, &reply);
	if( reply.what != B_REPLY )
		be_app->PostMessage(B_QUIT_REQUESTED);
	else
	{
		int32 winCt;
		reply.FindInt32("result", &winCt);
		if( winCt == 1 )
			be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

void
PrefsWin::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case QUERY_ADDED:
		{
			cout<<"In QUERY_ADDED\n";
			Preference pref(APP_SIG);
			pref.Load();
			int32 ct;
			cout<<"Queries: "<<ct<<endl;
			type_code type;
			if( pref.GetInfo("query", &type, &ct) == B_OK )
			{
				BMessage queryMsg;
				pref.FindMessage("query", ct - 1, &queryMsg);
				const char* queryStr, *queryName;
				dev_t vol;
				int32 showCount;
				queryMsg.FindString("name", &queryName);
				queryMsg.FindString("query", &queryStr);
				queryMsg.FindInt32("showCount", &showCount);
				queryMsg.FindInt32("volume", &vol);
				cout<<"Adding: "<<queryName<<" "<<queryStr
					<<" "<<vol<<" "<<showCount<<endl;
				AddQuery(queryName, queryStr, vol, showCount);
			}
			
		}	
			break;
		
		case ADD_QUERY:
			{
				EditWin* ew = new EditWin();
				ew->Show();
			}
			break;
		
/*		case QUERY_LIST_INV:
			
			break;*/
			
		default:
			BWindow::MessageReceived(msg);
	}
}

void
PrefsWin::AddQuery(const char* name, const char* query, dev_t vols, bool showMatches)
{
	BVolume vol(vols);
	char volName[B_FILE_NAME_LENGTH];
	vol.GetName(volName);
	
	CLVEasyItem* item = new CLVEasyItem;
	item->SetColumnContent(0, name);
	item->SetColumnContent(1, volName );
	item->SetColumnContent(2, showMatches ? "yes" : "no" );
	
	fCLV->AddItem(item);
}


/*
CLVCheckBox


CLVCheckBox::CLVCheckBox(bool selected)
	: CLVListItem(0, false, true, 18.0)
{
	fCB = new BCheckBox(
}

CLVCheckBox::~CLVCheckBox()
{
}

*/
