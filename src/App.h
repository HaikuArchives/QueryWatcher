#ifndef App_h
#define App_h

#include <Entry.h>
#include <Window.h>
#include <Application.h>
#include <StringView.h>
#include <View.h>
#include <Query.h>
#include <Node.h>
#include <String.h>

// eiman
#include <list>
#include <algorithm>

#if BUILD_QUERYWATCHER
	#define IMPEXP __declspec(dllexport)
#else
	#define IMPEXP __declspec(dllimport)
#endif


#define APP_SIG "application/x-vnd.NHS-QueryWatcher"

class ReplicantView;
class LabelView;
class ColorView;

// slaad
typedef list<BQuery *> querylist;

class App : public BApplication
{
public:
							App();
							~App();
	
	void 					MessageReceived(BMessage* msg);
	
	
};

class Window : public BWindow
{
public:
							Window(BRect rect);
							~Window();
	
	bool 					QuitRequested();
	void 					MessageReceived(BMessage* msg);
	
	void 					AddQuery(const entry_ref& path, const char* predicate, BRect rect);

private:
	ReplicantView*			fBackground;
};

IMPEXP class ReplicantView : public BView
{
public:
							ReplicantView(BRect frame);
							ReplicantView(BMessage* msg);
							~ReplicantView();

	void					AttachedToWindow();	
	status_t 				Archive(BMessage* msg, bool deep=true) const;
	static BArchivable* 	Instantiate(BMessage* msg);
	void 					AboutRequested();
	void 					MessageReceived(BMessage* msg);
};

IMPEXP class QueryView : public BView
{
public:
							QueryView(BRect frame, const char* title, const char* query, const entry_ref& r);
							QueryView(BMessage* msg);
							~QueryView();
	
	void 					AttachedToWindow();
	void 					Init(const char* title, const char* query);
	void 					MessageReceived(BMessage* msg);
	status_t 				Archive(BMessage* msg, bool deep=true) const;
	static 					BArchivable* Instantiate(BMessage* msg);
	void 					Reset();
	virtual void 			MouseDown(BPoint point);	
	
private:
	void 					UpdateDisplay();
	bool					CheckLastNodeCache(dev_t device, ino_t node, uint32 type);
	void					GetInitialEntries();
	// eiman
	bool					ShouldIgnore(BMessage*);
	
	int				fEntryCount;
	LabelView*		fLabelView;
	ColorView*		fColorView;
	entry_ref		fEntry;
	node_ref 		fLastQueryEntry;
	uint32			fLastQueryType;

	// slaad	
	querylist		fQueries;
	BString			fPredicate;
	
	// eiman
	list<node_ref>	fIgnoredMatches;
};

IMPEXP class LabelView : public BStringView
{
public:
							LabelView(BRect frame, const char* name, const char* text)
								: BStringView(frame, name, text) {}
							LabelView(BMessage* msg)
								: BStringView(msg) {}
	status_t				Archive(BMessage* msg, bool deep = true) const
							{ 	return BStringView::Archive(msg, deep); }
	static BArchivable*		Instantiate(BMessage* msg) 
							{ 	return validate_instantiation(msg, "LabelView") ? new LabelView(msg) : NULL; }							
	virtual void			MouseDown(BPoint point);	
};

IMPEXP class ColorView : public BView
{
public:
							ColorView(BRect frame, const char* name) 
								: BView(frame, name, B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW) {}
							
							ColorView(BMessage* msg)
								: BView(msg) {}
							
	status_t				Archive(BMessage* msg, bool deep = true) const
							{ 	return BView::Archive(msg, deep); }
	static BArchivable*		Instantiate(BMessage* msg) { return validate_instantiation(msg, "ColorView") ? new ColorView(msg) : NULL; }
	virtual void			MouseDown(BPoint point);					
};


#endif
