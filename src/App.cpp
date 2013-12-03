//#define DEBUG 1
#define BUILD_QUERYWATCHER 1
#include "App.h"

#ifdef DEBUG
#include <BeDC.h>
#include <Debug.h>
#include <String.h>
#endif

// eiman
#include <Path.h>

// slaad
#include <FindDirectory.h>

#include <Alert.h>
#include <Directory.h>
#include <fs_attr.h>
#include <File.h>
#include <Dragger.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <Volume.h>
#include <VolumeRoster.h>

#define RESET_QUERY 	'rset'
#define OPEN_QUERY_WIN 	'oqwn'
#define OPEN_SETTINGS_DIR 'opsd'

// slaad
const char kSettingsDir[] = "Nerd Herd Software/QueryWatcher";
const char kIgnoreDir[] = "Ignore";
const char kQueryDir[] = "Queries";

const char kTrackerMIME[] = "application/x-vnd.Be-TRAK";
const char *kTrackerQueryVolume = "_trk/qryvol1";

#include <be/kernel/fs_info.h>

// Yes, globals are bad. But, sssh!
typedef std::list<entry_ref> reflist;
reflist gIgnoreList;
BPath gSettingsPath;

struct query_load_info
{
	~query_load_info()
	{
		delete [] predicate;
	}
		
	entry_ref 		ref;
	const char* 	predicate;
	BRect 			rect;	
};

typedef std::list<BVolume> vollist;

char *ReadAttribute(BNode node, const char *attribute, int32 *length = NULL) {
	attr_info info;
	char *value = NULL;

	if (node.GetAttrInfo(attribute, &info) == B_OK) {
		value = (char *)calloc(info.size, sizeof(char));
		if (node.ReadAttr(attribute, info.type, 0, (void *)value, info.size) !=
			info.size) {
			
			free(value);
			value = NULL;
		};
		if (length) *length = info.size;
	};

	return value;
};

status_t ExtractQueryVolumes(BNode *node, vollist *volumes) {
	int32 length = 0;
	char *attr = ReadAttribute(*node, kTrackerQueryVolume, &length);
	BVolumeRoster roster;

	if (attr == NULL) {
		roster.Rewind();
		BVolume vol;
		
		while (roster.GetNextVolume(&vol) == B_NO_ERROR) {
			if ((vol.IsPersistent() == true) && (vol.KnowsQuery() == true)) {
				volumes->push_back(vol);
			};
		};
	} else {
		BMessage msg;
		msg.Unflatten(attr);

//		!*YOINK*!d from that project... with the funny little doggie as a logo...
//		OpenTracker, that's it!
			
		time_t created;
		off_t capacity;
		
		for (int32 index = 0; msg.FindInt32("creationDate", index, &created) == B_OK;
			index++) {
			
			if ((msg.FindInt32("creationDate", index, &created) != B_OK)
				|| (msg.FindInt64("capacity", index, &capacity) != B_OK))
				return B_ERROR;
		
			BVolume volume;
			BString deviceName = "";
			BString volumeName = "";
			BString fshName = "";
		
			if (msg.FindString("deviceName", &deviceName) == B_OK
				&& msg.FindString("volumeName", &volumeName) == B_OK
				&& msg.FindString("fshName", &fshName) == B_OK) {
				// New style volume identifiers: We have a couple of characteristics,
				// and compute a score from them. The volume with the greatest score
				// (if over a certain threshold) is the one we're looking for. We
				// pick the first volume, in case there is more than one with the
				// same score.
				int foundScore = -1;
				roster.Rewind();
				
				char name[B_FILE_NAME_LENGTH];
				
				while (roster.GetNextVolume(&volume) == B_OK) {
					if (volume.IsPersistent() && volume.KnowsQuery()) {
						// get creation time and fs_info
						BDirectory root;
						volume.GetRootDirectory(&root);
						time_t cmpCreated;
						fs_info info;
						if (root.GetCreationTime(&cmpCreated) == B_OK
							&& fs_stat_dev(volume.Device(), &info) == 0) {
							// compute the score
							int score = 0;
		
							// creation time
							if (created == cmpCreated)
								score += 5;
							// capacity
							if (capacity == volume.Capacity())
								score += 4;
							// device name
							if (deviceName == info.device_name)
								score += 3;
							// volume name
							if (volumeName == info.volume_name)
								score += 2;
							// fsh name
							if (fshName == info.fsh_name)
								score += 1;
		
							// check score
							if (score >= 9 && score > foundScore) {
								volume.GetName(name);
								volumes->push_back(volume);
							}
						}
					}
				}
			} else {
				// Old style volume identifiers: We have only creation time and
				// capacity. Both must match.
				roster.Rewind();
				while (roster.GetNextVolume(&volume) == B_OK)
					if (volume.IsPersistent() && volume.KnowsQuery()) {
						BDirectory root;
						volume.GetRootDirectory(&root);
						time_t cmpCreated;
						root.GetCreationTime(&cmpCreated);
						if (created == cmpCreated && capacity == volume.Capacity()) {
							volumes->push_back(volume);
						}
					}
			}
		};
	};

	return B_OK;	
};

App::App()
	: BApplication(APP_SIG)
{
	// slaad
	find_directory(B_USER_SETTINGS_DIRECTORY, &gSettingsPath, true);
	gSettingsPath.Append(kSettingsDir);

	BPath ignorePath(gSettingsPath);
	ignorePath.Append(kIgnoreDir);
	BDirectory ignoreDir(ignorePath.Path());
	if (ignoreDir.InitCheck() == B_OK) {
		entry_ref ref;
		while (ignoreDir.GetNextRef(&ref) == B_OK) {
			BEntry target(&ref, true);
			entry_ref ignore;
			target.GetRef(&ignore);
			
			gIgnoreList.push_back(ignore);
		};
	};
//	You can't symlink to Trash, so it's hardcoaded.
	entry_ref trash;
	if (get_ref_for_path("/boot/home/Desktop/Trash", &trash) == B_OK) {
		gIgnoreList.push_back(trash);
	};
	
	BPath queryPath(gSettingsPath);
	queryPath.Append(kQueryDir);
	BDirectory queryDir(queryPath.Path());
	bool showError = false;
	if ( queryDir.InitCheck() == B_OK )
	{
		int32 ctr;
		int32 entries = queryDir.CountEntries();
		query_load_info* loadInfo = new query_load_info[ entries ];
		for( ctr=0; ctr<entries; ctr++)
		{
			queryDir.GetNextRef( &(loadInfo[ctr].ref) );
			BFile file(&(loadInfo[ctr].ref), B_READ_ONLY);
			if( file.InitCheck() != B_OK )
			{
				showError = true;
				break;
			}
			attr_info ainfo;
			if( file.GetAttrInfo("_trk/qrystr", &ainfo) != B_OK )
			{
				showError = true;
				break;
			}
			loadInfo[ctr].predicate = new char[ainfo.size+1];
			if( file.ReadAttr("_trk/qrystr", B_STRING_TYPE, 0, (void*) loadInfo[ctr].predicate, ainfo.size) <= 0 )
			{
				showError = true;
				break;
			}
			#ifdef DEBUG
				BeDC dc("QueryWatcher");
				BString str;
				str<<loadInfo[ctr].ref.name<<" r-side: "<<be_plain_font->StringWidth(loadInfo[ctr].ref.name)+30;
				dc.SendMessage( str.String() );
			#endif
			loadInfo[ctr].rect.Set(0, ctr*16, be_plain_font->StringWidth(loadInfo[ctr].ref.name)+30, (ctr+1)*16);
		}
		if( ctr == 0 )
			showError = true;
		else
		{
			//find largest right-hand side of rect
			float r=0;
			for(int i=0; i<ctr; i++)
			{
				if( loadInfo[i].rect.right > r )
					r = loadInfo[i].rect.right;
			}
			
			#ifdef DEBUG
				BeDC dc("QueryWatcher");
				BString str;
				str<<"Largest r-side: "<<r;
				dc.SendMessage( str.String() );
			#endif
			
			Window* win = new Window( BRect(100,100, r + 107, ((ctr)*16) + 107 ));
			for( int32 loadCtr=0; loadCtr<ctr; loadCtr++)
				win->AddQuery( loadInfo[loadCtr].ref, loadInfo[loadCtr].predicate, loadInfo[loadCtr].rect );
			win->Show();
		}
		delete [] loadInfo;
	}
	else
		showError = true;
	
	if( showError )
	{
		BAlert* alert = new BAlert("Missing Queries", "No Tracker query files were found.\nIf this is your first time running QueryWatcher,\nsorry for the kludge, but you'd better read\nthe documentation first.", "Lame", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go();
		
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	}	
}

App::~App()
{
	
}

void
App::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		default: 
			BApplication::MessageReceived(msg);
	}
}





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
	
}

bool 
Window::QuitRequested()
{
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
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
		fEntry(r),
		fLastQueryType(B_OK)
{
	Init(title, query);
}

QueryView::QueryView(BMessage* msg)
	: BView(msg), fEntryCount(0), fLastQueryType(B_OK)
{
	const char* query;
	
	msg->FindString("QueryView:Predicate", &query);
	msg->FindRef("QueryView:Entry", &fEntry);
	
	Init(Name(), query);
}

QueryView::~QueryView()
{
	querylist::iterator qIt;
	for (qIt = fQueries.begin(); qIt != fQueries.end(); qIt++) {
		(*qIt)->Clear();
		delete (*qIt);
	};	
}

void
QueryView::AttachedToWindow()
{
	rgb_color parentColor = Parent()->ViewColor();
	SetViewColor(parentColor);
	fLabelView->SetViewColor(parentColor);
	
/*	BMessage debugmsg;
	rgb_color bgcolor = Parent()->ViewColor();
	debugmsg.AddInt8("red", bgcolor.red);
	debugmsg.AddInt8("green", bgcolor.green);
	debugmsg.AddInt8("blue", bgcolor.blue);
	debugmsg.PrintToStream();*/
	
	
	if( parentColor.red < 100 && parentColor.green < 100 && parentColor.blue < 100 )
		fLabelView->SetHighColor( 255, 255, 255 );

	GetInitialEntries();
	UpdateDisplay();
}


// eiman
// slaad - changed to use gIgnoreList
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
			
			reflist::iterator iter;
			
			iter = gIgnoreList.begin();
			
			while ( iter != gIgnoreList.end() )
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
		
		reflist::iterator iter;
		
		iter = gIgnoreList.begin();
		
		while ( iter != gIgnoreList.end() )
		{
			if ( (*iter) == parent_ref )
			{ // Parent is ignored, break
				node_ref ref;
				ref.node = node;
				ref.device = device;
				
				std::list<node_ref>::iterator iter;
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
			std::list<node_ref>::iterator iter;
			
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
		std::list<node_ref>::iterator iter;
		
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
	
	return result;
}



void
QueryView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		// eiman
		case B_NODE_MONITOR:
			{
				node_ref node;
				msg->FindInt32("device",&node.device);
				msg->FindInt64("node",&node.node);
				
				std::list<node_ref>::iterator iter;
				
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
				#ifdef DEBUG
				ASSERT(ret == B_OK);
				#endif
				ret = msg->FindInt64("node", &node);
				#ifdef DEBUG
				ASSERT(ret == B_OK);
				#endif
				int32 opcode;
				ret = msg->FindInt32("opcode", &opcode);
				#ifdef DEBUG
				ASSERT(ret == B_OK);
				#endif
				// eiman
				if( CheckLastNodeCache(device, node, opcode) && !ShouldIgnore(msg) )
				{
					if( opcode == B_ENTRY_CREATED )	
					{
						fEntryCount++;
						#ifdef DEBUG
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
						#ifdef DEBUG
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
				BMessenger trackerMsgr(kTrackerMIME);
				trackerMsgr.SendMessage(&openmsg);
			}
			break;

//		slaad		
		case OPEN_SETTINGS_DIR: {
			BMessage openmsg(B_REFS_RECEIVED);
			entry_ref ref;
			
			get_ref_for_path(gSettingsPath.Path(), &ref);
			openmsg.AddRef("refs", &ref);
			
			BMessenger(kTrackerMIME).SendMessage(&openmsg);
		} break;
		
		default:
			BView::MessageReceived(msg);	
	}
}

status_t 
QueryView::Archive(BMessage* msg, bool deep) const
{
	msg->AddString("add_on", APP_SIG);
	msg->AddRef("QueryView:Entry", &fEntry);
	msg->AddString("QueryView:Predicate", fPredicate);
	
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
	rgb_color red, green;
	red.red = 179;
	red.green = 0;
	red.blue = 0;
	
	green.green = 195;
	green.red = 0;
	green.blue = 0;
	
	if( fEntryCount == 0 )
		fColorView->SetViewColor(red);
	else
		fColorView->SetViewColor(green);

	fColorView->Invalidate();
}


//returns false if the node matches the previously recorded node (last node sent via query update)
//returns true otherwise, and updates the node cache
bool
QueryView::CheckLastNodeCache(dev_t device, ino_t node, uint32 type)
{
	if( device == fLastQueryEntry.device && node == fLastQueryEntry.node && type == fLastQueryType )
	{
		#ifdef DEBUG
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
	
	rgb_color red;
	red.red = 179;
	red.green = 0;
	red.blue = 0;
	
//	slaad		
	fPredicate = query;	

	BRect labelRect = Bounds();
	labelRect.InsetBy(2, 2);
	labelRect.left = 18;
	fLabelView = new LabelView(labelRect, "LabelView", title);
	AddChild(fLabelView);
	
	BRect colorRect = Bounds();
	colorRect.InsetBy(4, 4);
	colorRect.right = 14;
	fColorView = new ColorView( colorRect, "ColorView");
	fColorView->SetViewColor(red);
	AddChild(fColorView);
}

void
QueryView::GetInitialEntries()
{	
	fEntryCount = 0;	
	entry_ref ref;

//	slaad
	BNode n(&ref);
	vollist vols;
	
	ExtractQueryVolumes(&n, &vols);
	vollist::iterator vIt;
	
	for (vIt = vols.begin(); vIt != vols.end(); vIt++) {
		BQuery *query = new BQuery();
		query->SetVolume(&(*vIt));
		query->SetPredicate(fPredicate.String());
		query->SetTarget(this);
		query->Fetch();
	
		while( query->GetNextRef(&ref) == B_OK )
		{
			// eiman
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
		
		fQueries.push_back(query);
	};
	
	#ifdef DEBUG
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
//	slaad
	
	querylist::iterator qIt;
	for (qIt = fQueries.begin(); qIt != fQueries.end(); qIt++) {
		(*qIt)->Clear();
		delete (*qIt);
	};
	
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
		
		popup->AddItem(new BMenuItem("Open settings directory", new BMessage(OPEN_SETTINGS_DIR)));
		popup->AddSeparatorItem();
		
		popup->AddItem( new BMenuItem("Open query results", new BMessage(OPEN_QUERY_WIN) ) );
		popup->AddItem( new BMenuItem("Reset query", new BMessage(RESET_QUERY) ) );
		status_t ret;
		ret = popup->SetTargetForItems(this);		
		#ifdef DEBUG
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

void 
ColorView::MouseDown(BPoint point)
{
	ConvertToParent(&point);
	((QueryView*)Parent())->MouseDown(point);	
}


int main()
{
	#ifdef DEBUG
	SET_DEBUG_ENABLED(true);
	#endif
	
	App app;
	app.Run();
	
	return 0;
}
