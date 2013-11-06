#define BUILD_QUERYWATCHER 1
#define QWDEBUG 1
#define DEBUG 1

#define TRACKER_QUERYFILE_ATTR_NAME "_trk/qrystr"

#include "App.h"
#include "AppConstants.h"
#include "Win.h"

#include <Alert.h>
#include <fs_attr.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Volume.h>
#include <VolumeRoster.h>

//STL
#include <cstdio>
using namespace std;

//PrefServer by Marco Nelissen
#include "Preference.h"

#ifdef QWDEBUG
#include <BeDC.h>
#include <Debug.h>
#endif

#define RESET_QUERY 	'rset'
#define OPEN_QUERY_WIN 	'oqwn'

Window::Window(BRect rect)
	: BWindow( rect, "QueryWatcher", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fBackground = new ReplicantView( Bounds() );
	fBackground->SetViewColor(B_TRANSPARENT_COLOR);

	BRect draggerFrame = Bounds();
	draggerFrame.top = draggerFrame.bottom - 7;
	draggerFrame.left = draggerFrame.right - 7;

	BDragger* dragger = new BDragger(draggerFrame, fBackground);
	dragger->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fBackground);
	fBackground->AddChild(dragger);
	
	
}

Window::~Window()
{
	Preference pref(APP_SIG);
	pref.Load();
	type_code t;
	if( pref.GetInfo( "mainwin:frame", &t ) == B_OK )
		pref.ReplaceRect("mainwin:frame", Frame());
	else
		pref.AddRect("mainwin:frame", Frame());	
	pref.Save();
}

bool 
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
Window::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		default:
			BWindow::MessageReceived(msg);
	}
}

void
Window::AddQuery(const entry_ref& path, const char* predicate, BRect rect)
{
	fBackground->AddChild( new QueryView(rect, path.name, predicate, path) );
}

void
Window::AddDefaultBackground()
{
	fBackground->AddChild(new DefaultBGView( fBackground->Bounds() ) );
}

DefaultBGView::DefaultBGView(BRect frame)
	: BView( frame, "AcceptsDropView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) ) ;
}

void
DefaultBGView::MessageReceived(BMessage* msg)
{
	if( msg->WasDropped() )
	{
		//TODO: split this between moving and adding; both are dropped messages
		int32 refCount;
		type_code type;
		if( msg->GetInfo("refs", &type, &refCount) == B_OK )
		{
			entry_ref ref;
			for( int32 refCtr=0; refCtr < refCount; refCtr++ )
			{
				msg->FindRef( "refs", refCtr, &ref );
				BFile queryFile( &ref, B_READ_ONLY );
				attr_info ainfo;
				if( queryFile.GetAttrInfo( TRACKER_QUERYFILE_ATTR_NAME, &ainfo ) == B_OK )
				{
					char* queryString = new char[ ainfo.size + 1 ];
					if( queryFile.ReadAttr( TRACKER_QUERYFILE_ATTR_NAME, B_STRING_TYPE, 0, (void*) queryString, ainfo.size+1) == B_OK )
					{
						
					}
				}
			}
		}
	}
	else
		BView::MessageReceived( msg );
}

void
DefaultBGView::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	
}

DefaultBGView::~DefaultBGView()
{}

//todo: optimize this by avoiding drawing outside the clipping rect.  Yeah right.
void
DefaultBGView::Draw(BRect clipRect)
{
	SetFont(be_bold_font);
	DrawString( APP_NAME_VER, BPoint(3,14));
	SetFont(be_plain_font);
	DrawString( "Drag and drop queries here to begin.", BPoint(3, 28));
	DrawString( "Right click for preferences.", BPoint(3, 40) );
}

status_t
DefaultBGView::Archive(BMessage* msg, bool deep) const 
{ 
	if( msg->AddString("add_on", APP_SIG) != B_OK )
		return B_ERROR;
	return BView::Archive(msg, deep);
}


ReplicantView::ReplicantView(BRect frame)
	: BView( frame, "QueryWatcher", B_FOLLOW_ALL, B_WILL_DRAW)
{
}

ReplicantView::~ReplicantView()
{}

void
ReplicantView::AttachedToWindow()
{
	BView* parent = Parent();
	if( parent == NULL )
		SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	else
		SetViewColor( Parent()->ViewColor() );

/*	BView* child;
	for(int32 ctr=0; ctr< CountChildren(); ctr++)
	{
		child = ChildAt(ctr);
		if( dynamic_cast<BDragger*> (child) != NULL )
		{
			BPopUpMenu* popup = ((BDragger*)child)->PopUp();
			
			ASSERT( parent == NULL || (parent != NULL && popup != NULL) );
			
			if( popup != NULL && popup->ItemAt(3) == NULL )
					popup->AddItem( new BMenuItem("Refresh queries", new BMessage(RESET_QUERY)), 1 );
			break;
		}
	}*/
}

status_t 
ReplicantView::Archive(BMessage* msg, bool deep) const
{
	msg->AddString("add_on", APP_SIG);
	return BView::Archive(msg, deep);
}

ReplicantView::ReplicantView(BMessage* msg)
	: BView(msg)
{
	
}

BArchivable* ReplicantView::Instantiate(BMessage* msg)
{
	if( ! validate_instantiation(msg, "ReplicantView") )
		return NULL;
	return new ReplicantView(msg);	
}

void
ReplicantView::AboutRequested()
{
	BAlert* alert = new BAlert("QueryWatcher", "QueryWatcher 1.0\nBy Michael Armida\ninfo@nerdherdsoftware.com", "Gee...Thanks");
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}

void
ReplicantView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
/*		case RESET_QUERIES:
			{
				for(int32 viewCtr=0; viewCtr < CountChildren(); viewCtr ++ )
				{
					BView* child = ChildAt(viewCtr);
					if( dynamic_cast<QueryView*>(child) != NULL )
					{
						((QueryView*)child)->Reset();
					}
				}
			}
			break;*/
		
		default:
			BView::MessageReceived(msg);
	}
}




QueryView::QueryView(BRect frame, const char* title, const char* query, const entry_ref& r)
	: 	BView( frame, title, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		fEntryCount(0),
		fQuery(new BQuery),
		fEntry(r),
		fLastQueryType(B_OK)
{
	Init(title, query);
}

QueryView::QueryView(BMessage* msg)
	: BView(msg), fEntryCount(0), fQuery(new BQuery), fLastQueryType(B_OK)
{
	const char* query;
	msg->FindString("QueryView:Predicate", &query);
	
	msg->FindRef("QueryView:Entry", &fEntry);
	
	Init(Name(), query);
}

QueryView::~QueryView()
{
	delete fQuery;
}

void
QueryView::AttachedToWindow()
{
	rgb_color parentColor = Parent()->ViewColor();
	SetViewColor(parentColor);
	fLabelView->SetViewColor(parentColor);
	
	if( parentColor.red < 100 && parentColor.green < 100 && parentColor.blue < 100 )
		fLabelView->SetHighColor( 255, 255, 255 );
	
	status_t err;
	BMessenger target(this, NULL, &err);
	status_t ret = fQuery->SetTarget(target);
	#ifdef QWDEBUG
	ASSERT_WITH_MESSAGE(ret == B_OK, strerror(ret));
	#endif
	ret = fQuery->Fetch();
	#ifdef QWDEBUG
	ASSERT_WITH_MESSAGE(ret == B_OK, strerror(ret));
	#endif

	GetInitialEntries();		
	
	UpdateDisplay();
}


bool
QueryView::ShouldIgnore( BMessage * msg )
{
	bool result=false;
	
	dev_t device;
	ino_t node;
	ino_t directory;
	int32 opcode;
	const char * name;
	
	msg->FindInt32("device",&device);
	msg->FindInt64("node",&node);
	msg->FindInt64("directory",&directory);
	msg->FindInt32("opcode",&opcode);
	msg->FindString("name",&name);
	
	// create list of dirs to ignore
	// this should be kept in a class-variable and
	// created in Init(), but a compiler bug prevents me from doing that
	list<entry_ref>	fIgnoredDirRefs;
		
	entry_ref ref;
	if ( get_ref_for_path("/boot/home/Desktop/Trash",&ref) == B_OK )
		fIgnoredDirRefs.push_back(ref);
	if ( get_ref_for_path("/boot/home/mail/Spam",&ref) == B_OK )
		fIgnoredDirRefs.push_back(ref);
		
	if ( opcode == B_ENTRY_CREATED )
	{
		// Entry for matching file
		entry_ref match_ref(device,directory,name);
		BEntry match(&match_ref);
						
		BEntry parent;
					
		if ( match.GetParent(&parent) == B_OK )
		{
			entry_ref parent_ref;
			parent.GetRef(&parent_ref);
			
			list<entry_ref>::iterator iter;
			
			iter = fIgnoredDirRefs.begin();
			
			while ( iter != fIgnoredDirRefs.end() )
			{
				if ( (*iter) == parent_ref )
				{ // Parent is ignored, break
					node_ref ref;
					ref.node = node;
					ref.device = device;
					fIgnoredMatches.push_back( ref );
					result = true;
					break;
				}
				
				iter++;
			}
		}
		
		// start watching node
		node_ref ref;
		ref.node = node;
		ref.device = device;
		
		watch_node( &ref, B_WATCH_NAME, BMessenger(this) );
	}
	
	if ( opcode == B_ENTRY_MOVED )
	{ // from node-monitoring, not query
		// Get entry_ref to file
		ino_t to_dir;
		
		msg->FindInt64("to directory",&to_dir);
		entry_ref to_ref(device,to_dir,name);
		
		// get parent
		BEntry entry(&to_ref);
		BEntry parent;
		entry.GetParent(&parent);
		
		entry_ref parent_ref;
		parent.GetRef(&parent_ref);
		
		list<entry_ref>::iterator iter;
		
		iter = fIgnoredDirRefs.begin();
		
		while ( iter != fIgnoredDirRefs.end() )
		{
			if ( (*iter) == parent_ref )
			{ // Parent is ignored, break
				node_ref ref;
				ref.node = node;
				ref.device = device;
				
				list<node_ref>::iterator iter;
				iter = find(fIgnoredMatches.begin(), fIgnoredMatches.end(), ref);
				
				if ( iter == fIgnoredMatches.end() )
				{ // make sure we don't ignore twice (moving between two ignored dirs)
					fIgnoredMatches.push_back( ref );
				}
				
				result = true;
				break;
			}
			
			iter++;
		}
		
		if ( !result )
		{
			// remove node from ignore list if present
			list<node_ref>::iterator iter;
			
			node_ref ref;
			ref.node = node;
			ref.device = device;
			
			iter = find(fIgnoredMatches.begin(), fIgnoredMatches.end(), ref);
			
			if ( iter != fIgnoredMatches.end() )
			{ // found ignored
				fIgnoredMatches.erase(iter);
			}
		}
	}
	
	if ( opcode == B_ENTRY_REMOVED )
	{
		// remove from fIgnoredMatches if there
		list<node_ref>::iterator iter;
		
		node_ref ref;
		ref.node = node;
		ref.device = device;
		
		iter = find(fIgnoredMatches.begin(), fIgnoredMatches.end(), ref);
		
		if ( iter != fIgnoredMatches.end() )
		{ // found ignored
			fIgnoredMatches.erase(iter);
			result = true;
		}
		
		watch_node( &ref, B_STOP_WATCHING, BMessenger(this) );
	}
	// end Filter out Trash
	
	return result;
}



void
QueryView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_NODE_MONITOR:
			{
				node_ref node;
				msg->FindInt32("device",&node.device);
				msg->FindInt64("node",&node.node);
				
				list<node_ref>::iterator iter;
				
				iter = find(fIgnoredMatches.begin(),fIgnoredMatches.end(),node);
				
				bool was_ignored = (iter != fIgnoredMatches.end());
				bool should_ignore = ShouldIgnore(msg);
				
				if (  !was_ignored && should_ignore )
				{ // was not ignored
					fEntryCount--;
				}
				
				if ( was_ignored && !should_ignore )
				{ // was ignored
					fEntryCount++;
				}
			}
			UpdateDisplay();
			break;
			
		case B_QUERY_UPDATE:
			{
				int32 device;
				int64 node;
				status_t ret;
				ret = msg->FindInt32("device", &device);
				#ifdef QWDEBUG
				ASSERT(ret == B_OK);
				#endif
				ret = msg->FindInt64("node", &node);
				#ifdef QWDEBUG
				ASSERT(ret == B_OK);
				#endif
				int32 opcode;
				ret = msg->FindInt32("opcode", &opcode);
				#ifdef QWDEBUG
				ASSERT(ret == B_OK);
				#endif


				if( CheckLastNodeCache(device, node, opcode) && !ShouldIgnore(msg) )
				{
					if( opcode == B_ENTRY_CREATED )	
					{
						fEntryCount++;
						#ifdef QWDEBUG
							BeDC dc("QueryWatcher");
							BString str("Match found for query: ");
							str<<Name()<<" ("<<fEntryCount<<')';
							dc.SendMessage( str.String() );
						#endif
					}
					if( opcode == B_ENTRY_REMOVED )
					{
						if( fEntryCount > 0 )
							fEntryCount--;					
						#ifdef QWDEBUG
							BeDC dc("QueryWatcher");
							BString str("Match removed for query: ");
							str<<Name()<<" ("<<fEntryCount<<')';
							dc.SendMessage( str.String() );
						#endif
					}
				}
			}
//			cout<<fLabelView->Text()<<": "<<fEntryCount<<endl;
			UpdateDisplay();

			break;
		case RESET_QUERY:
			Reset();
			break;
			
		case OPEN_QUERY_WIN:
			{
				//be_roster->Launch(&fEntry);  //doesn't work for some reason...
				BMessage openmsg(B_REFS_RECEIVED);
				openmsg.AddRef("refs", &fEntry);
				BMessenger trackerMsgr( "application/x-vnd.Be-TRAK" );
				trackerMsgr.SendMessage(&openmsg);
			}
			break;
		
		default:
			BView::MessageReceived(msg);	
	}
}

status_t 
QueryView::Archive(BMessage* msg, bool deep) const
{
	msg->AddString("add_on", APP_SIG);
	msg->AddRef("QueryView:Entry", &fEntry);
	
	char* pred = new char[ fQuery->PredicateLength() +1 ];
	if( fQuery->GetPredicate(pred, fQuery->PredicateLength() ) != B_OK )
		return B_ERROR;
	msg->AddString("QueryView:Predicate", pred);
	delete pred;
	
	return BView::Archive(msg, deep);
}

BArchivable* 
QueryView::Instantiate(BMessage* archive)
{
	if( ! validate_instantiation( archive, "QueryView" ) )
		return NULL;
	return new QueryView(archive);
}

void
QueryView::UpdateDisplay()
{
	if( fEntryCount == 0 )
		fColorView->SetColor(ColorView::Red);
	else
		fColorView->SetColor(ColorView::Green);

	fColorView->SetMatchCount(fEntryCount);
	
	//fColorView->Invalidate();
}


//returns false if the node matches the previously recorded node (last node sent via query update)
//returns true otherwise, and updates the node cache
bool
QueryView::CheckLastNodeCache(dev_t device, ino_t node, uint32 type)
{
	if( device == fLastQueryEntry.device && node == fLastQueryEntry.node && type == fLastQueryType )
	{
		#ifdef QWDEBUG
			BeDC dc("QueryWatcher");
			dc.SendMessage("Caught duplicate query notification error...");
		#endif
		return false;
	}
	fLastQueryEntry.device = device;
	fLastQueryEntry.node = node;
	fLastQueryType = type;
	return true;
}

void 
QueryView::Init(const char* title, const char* query)
{
	//SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//	SetViewColor(B_TRANSPARENT_COLOR);
	BVolumeRoster vroster;
	BVolume vol;
	vroster.GetBootVolume(&vol);
	fQuery->SetVolume(&vol);
	fQuery->SetPredicate(query);
	
	BRect labelRect = Bounds();
	labelRect.InsetBy(2, 2);
	labelRect.left = 18;
	fLabelView = new LabelView(labelRect, "LabelView", title);
	AddChild(fLabelView);
	
	BRect colorRect = Bounds();
	colorRect.InsetBy(4, 4);
	colorRect.right = 14;
	fColorView = new ColorView( colorRect, "ColorView");
	AddChild(fColorView);
}

void
QueryView::GetInitialEntries()
{	
	fEntryCount = 0;
	
	entry_ref ref;
	while( fQuery->GetNextRef(&ref) == B_OK )
	{
		BEntry entry(&ref);
		node_ref node;
		entry.GetNodeRef(&node);
		
		BMessage msg;
		msg.AddInt32("opcode",B_ENTRY_CREATED);
		msg.AddString("name",ref.name);
		msg.AddInt64("directory",ref.directory);
		msg.AddInt32("device",ref.device);
		msg.AddInt64("node",node.node);
		if ( !ShouldIgnore(&msg) )
		{
			fEntryCount++;
		}
	}
	
	#ifdef QWDEBUG
	BeDC dc("QueryWatcher");
	BString str;
	str<<Name()<<" initial count: "<<fEntryCount;
	dc.SendMessage(str.String());
	#endif

	UpdateDisplay();
}

void
QueryView::Reset()
{	
	char* predicate = new char[fQuery->PredicateLength()+1];
	if( fQuery->GetPredicate(predicate, fQuery->PredicateLength()) != B_OK )
	{
		delete [] predicate;
		return;
	}
	
	fQuery->Clear();
	
	BVolumeRoster vroster;
	BVolume vol;
	vroster.GetBootVolume(&vol);
	fQuery->SetVolume(&vol);
	fQuery->SetPredicate(predicate);
	
	status_t err;
	BMessenger target(this, NULL, &err);
	status_t ret = fQuery->SetTarget(target);
	#ifdef QWDEBUG
	ASSERT_WITH_MESSAGE(ret == B_OK, strerror(ret));
	#endif
	ret = fQuery->Fetch();
	#ifdef QWDEBUG
	ASSERT_WITH_MESSAGE(ret == B_OK, strerror(ret));
	#endif
		
	GetInitialEntries();
}

void 
QueryView::MouseDown(BPoint point)
{
	BPoint cursorpoint;
	uint32 buttonmask;
	GetMouse(&cursorpoint, &buttonmask);
	if( buttonmask & B_PRIMARY_MOUSE_BUTTON )
	{
		//pop-up query results window
		BMessenger messenger(this);
		messenger.SendMessage(OPEN_QUERY_WIN);
		return;	//buttons aren't mutually exclusive, so we favor the primary.
	}
	if( buttonmask & B_SECONDARY_MOUSE_BUTTON )
	{
		//pop-up context menu
		BPopUpMenu* popup = new BPopUpMenu("QueryWatcher");
		
		BMenuItem* headeritem = new BMenuItem( Name(), NULL );
		headeritem->SetEnabled(false);
		popup->AddItem(headeritem);
		popup->AddSeparatorItem();
		
		popup->AddItem( new BMenuItem("Open query results", new BMessage(OPEN_QUERY_WIN) ) );
		popup->AddItem( new BMenuItem("Reset query", new BMessage(RESET_QUERY) ) );
		
		BMenuItem* mi_prefs = new BMenuItem("Preferences", new BMessage(OPEN_PREFS_WIN) );
		mi_prefs->SetTarget( be_app_messenger );
		popup->AddSeparatorItem();
		popup->AddItem(mi_prefs);
		
		status_t ret;
		ret = popup->SetTargetForItems(this);		
		#ifdef QWDEBUG
		ASSERT(ret == B_OK);
		#endif

		ConvertToScreen(&point);
		popup->Go(point, true, false, true);		
		delete popup;
	}
}


void
LabelView::MouseDown(BPoint point)
{
	ConvertToParent(&point);
	((QueryView*)Parent())->MouseDown(point);	
}

ColorView::ColorView(BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW)
{
}

void
ColorView::InitStatic()
{	
	fRed.red = 179;
	fRed.green = 0;
	fRed.blue = 0;	
	
	fGreen.green = 195;
	fGreen.red = 0;
	fGreen.blue = 0;
	
	#ifdef QWDEBUG
	BeDC dc("QueryWatcher");
	dc.SendMessage("ColorView::InitStatic() called");

	dc.SendInt( int32(ColorView::fRed.red), "Red" );
	dc.SendInt( int32(ColorView::fGreen.green)  , "Green");
	#endif

}

//this useless function was added to keep the static rgb_color vars private
void
ColorView::SetColor( color_t color )
{
	switch(color)
	{
		case Red:
			SetViewColor(ColorView::fRed);
			break;
		case Green:	
			SetViewColor(ColorView::fGreen);
			break;	
		#ifdef QWDEBUG
		default:
			BeDC dc("QueryWatcher");
			dc.SendMessage("Unknown color enum passed to ColorView::SetColor()", DC_ERROR);
		#endif
	}
}

void 
ColorView::MouseDown(BPoint point)
{
	ConvertToParent(&point);
	((QueryView*)Parent())->MouseDown(point);	
}

void
ColorView::Draw( BRect rect )
{
/*	#ifdef QWDEBUG
	BeDC dc("QueryWatcher");
	rgb_color vc = ViewColor();
	dc.SendInt( int32(vc.red), "ViewColor - red");
	dc.SendInt( int32(vc.green), "ViewColor - green");
	dc.SendInt( int32(vc.blue), "ViewColor - blue");
	#endif
	
	SetHighColor( ViewColor() );
	FillRect( Bounds() );*/
	
	if ( fMatchCount > 0 )
	{
		char text[5];
			
		if ( fMatchCount < 10 )
		{
			sprintf(text,"%d",fMatchCount);
		} else 
		{
			strcpy(text,"9+");
		}
		
		SetHighColor( 255,255,255 );
		SetLowColor( ViewColor() );
		SetFontSize(9);
		
		DrawString( text, BPoint( Bounds().right/2 - StringWidth(text)/2 + 1,Bounds().bottom ) );
	}
}

void
ColorView::SetMatchCount( int count )
{
	fMatchCount = count;
	Invalidate();
}
