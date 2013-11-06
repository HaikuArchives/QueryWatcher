#ifndef EditWin_h
#define EditWin_h

#include <Application.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Window.h>

#define EDITWIN_OK 'ew01'
#define EDITWIN_CANCEL 'ew02'
#define EDITWIN_NAMECHANGE 'ew03'
#define EDITWIN_QUERYCHANGE 'ew04'
#define EDITWIN_VOLSELECT 'ew05'
#define RESULTS_CB_CHANGE 'ew06'
#define EDITWIN_ALLVOLS_SEL 'ew07'

#define ALLVOLS -1

class EditWin : public BWindow
{
public:
						EditWin(); //new query
						EditWin(int qryIndex); //edit
						~EditWin();
						
	void 				MessageReceived(BMessage* msg);

private:
	void 				Init(); //setup window contents
	
	
	BTextControl		*fName, *fQuery;
	BPopUpMenu* 		fVolPopup;
	BCheckBox* 			fShowResults;
	
	int32				fCurDevice;
	bool				fCurShowCount;
};

#endif
