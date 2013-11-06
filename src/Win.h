#ifndef Win_h
#define Win_h


#include <Entry.h>
#include <Window.h>
#include <StringView.h>
#include <View.h>
#include <Query.h>
#include <Node.h>


//STL
#include <list>
#include <algorithm>
using namespace std;


#if BUILD_QUERYWATCHER
	#define IMPEXP __declspec(dllexport)
#else
	#define IMPEXP __declspec(dllimport)
#endif

class Window;

class QueryView;
class ReplicantView;
class LabelView;
class ColorView;
class AcceptsDropView;
class DefaultBGView;

class Window : public BWindow
{
public:
							Window(BRect rect);
							~Window();
	
	bool 					QuitRequested();
	void 					MessageReceived(BMessage* msg);
	
	void 					AddQuery(const entry_ref& path, const char* predicate, BRect rect);
	void					AddDefaultBackground();

private:
	ReplicantView*			fBackground;
};


IMPEXP class DefaultBGView : public BView
{
public:
							DefaultBGView(BRect frame);
							~DefaultBGView();
							DefaultBGView(BMessage* msg)
								: BView(msg) {}
	void					Draw(BRect updateRect);
	virtual	void			MessageReceived(BMessage* msg);
	virtual void			MouseMoved(BPoint point, uint32 transit, const BMessage* msg);

	status_t				Archive(BMessage* msg, bool deep=true) const;
	static BArchivable*		Instantiate(BMessage* msg)
							{	
								return validate_instantiation(msg, "DefaultBGView") ? new DefaultBGView(msg) : NULL;
							}
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
	bool					ShouldIgnore(BMessage*);
	


	int				fEntryCount;
	BQuery* 		fQuery;
	LabelView*		fLabelView;
	ColorView*		fColorView;
	entry_ref		fEntry;
	node_ref 		fLastQueryEntry;
	uint32			fLastQueryType;
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
	enum 					color_t
	{
		Red,
		Green	
	};
	
public:
							ColorView(BRect frame, const char* name);
							ColorView(BMessage* msg)
								: BView(msg) {}
							
	status_t				Archive(BMessage* msg, bool deep = true) const
							{ 	return BView::Archive(msg, deep); }
	static BArchivable*		Instantiate(BMessage* msg) { return validate_instantiation(msg, "ColorView") ? new ColorView(msg) : NULL; }
	virtual void			MouseDown(BPoint point);
	virtual void			Draw( BRect );
	void					SetMatchCount( int );
	void					SetColor( color_t color );
	static void				InitStatic();

private:
	int			fMatchCount;

	static rgb_color
				fRed, fGreen;

};



#endif
