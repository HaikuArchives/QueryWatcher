#ifndef PrefsWin_h
#define PrefsWin_h

#include "CLVListItem.h"
#include "ColumnListView.h"

#include <CheckBox.h>
#include <Window.h>

#define REORDER_QUERY 'pw01'
#define ADD_QUERY 'pw02'
#define DEL_QUERY 'pw03'
#define QUERY_LIST_INV 'pw04'
#define SHOW_MAINWIN 'pw05'
#define QUERY_ADDED 'pw06'
#define REFRESH_QUERIES 'pw07'

/*
class CLVCheckBox : public CLVListItem
{
public:
						CLVCheckBox(bool selected);
						~CLVCheckBox();

	void				ColumnWidthChanged(int32 colIndex, int32 colWidth, ColumnListView* view);
	void				DrawItemColumn(BView* owner, BRect itemColumnRect, int32 colIndex, bool complete = 0);
	
	

private:
	BCheckBox*			fCB;						
}; */


class PrefsWin : public BWindow
{
public:
						PrefsWin();
						~PrefsWin();
	void				MessageReceived(BMessage* msg);


private:
						//owns vols
	void				AddQuery(const char* name, const char* query, dev_t vols, bool showMatches);


	ColumnListView* 	fCLV;	
};


#endif
