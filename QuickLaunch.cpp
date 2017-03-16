/*
 * Copyright 2010-2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 *
 * A graphical launch panel finding an app via a query.
 */

#include "DeskButton.h"
#include "QLFilter.h"
#include "QuickLaunch.h"

#include <Deskbar.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"

const char *kApplicationSignature = "application/x-vnd.humdinger-quicklaunch";


QLApp::QLApp()
	:
	BApplication(kApplicationSignature)
{
	fSettings = new QLSettings();
	fSetupWindow = new SetupWindow(fSettings->GetSetupWindowFrame());
	fMainWindow = new MainWindow();
}


QLApp::~QLApp()
{
	BMessenger messengerMain(fMainWindow);
	if (messengerMain.IsValid() && messengerMain.LockTarget()) {
		fSettings->SetSearchTerm(fMainWindow->GetSearchString());
		fMainWindow->Quit();
	}
	delete fSettings;

	BMessenger messengerSetup(fSetupWindow);
	if (messengerSetup.IsValid() && messengerSetup.LockTarget())
		fSetupWindow->Quit();
}


#pragma mark -- BApplication Overrides --


void
QLApp::AboutRequested()
{
	BString text = B_TRANSLATE_COMMENT(
		"QuickLaunch %version%\n"
		"\twritten by Humdinger\n"
		"\tCopyright %years%\n\n"
		"QuickLaunch quickly starts any installed application. "
		"Just enter the first few letters of its name and choose "
		"from a list of all found programs.\n",
		"Don't change the variables %years% and %version%.");
	text.ReplaceAll("%version%", "v1.0");
	text.ReplaceAll("%years%", "2010-2017");

	BAlert *alert = new BAlert("about", text.String(),
		B_TRANSLATE("Thank you"));

	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);
	view->GetFont(&font);
	font.SetSize(font.Size()+4);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 11, &font);
	alert->Go();
}


void
QLApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SETUP_BUTTON:
		{
			if (fSetupWindow->IsHidden()) {
				SetWindowsFeel(0);
				fSetupWindow->Show();
			}
			else {
				fSetupWindow->Hide();
			}
			break;
		}
		case DESKBAR_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetDeskbar(value);
			if (value)
				_AddToDeskbar();
			else
				_RemoveFromDeskbar();
			break;
		}
		case VERSION_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetShowVersion(value);
			if (!fMainWindow->fListView->IsEmpty()) {
				fMainWindow->LockLooper();
				fMainWindow->fListView->Invalidate();
				fMainWindow->UnlockLooper();
			}
			break;
		}
		case PATH_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetShowPath(value);
			if (!fMainWindow->fListView->IsEmpty()) {
				fMainWindow->LockLooper();
				fMainWindow->fListView->Invalidate();
				fMainWindow->UnlockLooper();
			}
			break;
		}
		case DELAY_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetDelay(value);
			fMainWindow->LockLooper();
			fMainWindow->PostMessage('fltr');
			fMainWindow->UnlockLooper();
			break;
		}
		case SAVESEARCH_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetSaveSearch(value);
			break;
		}
		case SINGLECLICK_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetSingleClick(value);
			break;
		}
		case ONTOP_CHK:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetOnTop(value);
			break;
		}
		case IGNORE_CHK:
		{
			if (fSetupWindow->fIgnoreList->IsEmpty()) {
				fSetupWindow->LockLooper();
				fSetupWindow->fChkIgnore->SetValue(false);
				fSettings->SetShowIgnore(false);
				fSetupWindow->UnlockLooper();
				break;
			}
			int32 value;
			message->FindInt32("be:value", &value);
			fSettings->SetShowIgnore(value);
			if (!fSetupWindow->fIgnoreList->IsEmpty()) {
 				fSetupWindow->fChkIgnore->SetValue(value);
				if (!fMainWindow->fListView->IsEmpty()) {
	 				fMainWindow->fListView->LockLooper();
	 				const char *searchString = fMainWindow->GetSearchString();
					fMainWindow->BuildList(searchString);
					fMainWindow->fListView->UnlockLooper();
				}
			}
			break;
		}
		case FILEPANEL:
		{
			if (!fSetupWindow->fIgnoreList->IsEmpty()) {
				fSetupWindow->LockLooper();
				fSetupWindow->fChkIgnore->SetValue(true);
				fSettings->SetShowIgnore(true);
				fSetupWindow->UnlockLooper();
			} else {
				fSetupWindow->LockLooper();
				fSetupWindow->fChkIgnore->SetValue(false);
				fSettings->SetShowIgnore(false);
				fSetupWindow->UnlockLooper();
			}
			if (fMainWindow->GetStringLength() > 0) {

				fMainWindow->fListView->LockLooper();
				float position = fMainWindow->GetScrollPosition();
				const char *searchString = fMainWindow->GetSearchString();
				fMainWindow->BuildList(searchString);
				fMainWindow->SetScrollPosition(position);
				fMainWindow->fListView->UnlockLooper();
			}
			break;
		}
		default:
		{
			BApplication::MessageReceived(message);
			break;
		}
	}
}


bool
QLApp::QuitRequested()
{
	return true;
}


void
QLApp::ReadyToRun()
{
	BRect frame = fSettings->GetMainWindowFrame();
	fMainWindow->MoveTo(frame.LeftTop());
	fMainWindow->ResizeTo(frame.right - frame.left, 0);
	fMainWindow->Show();

	fSettings->InitIgnoreList();
	fSetupWindow->Hide();
	fSetupWindow->Show();

	if (fSettings->GetSaveSearch()) {
		BMessenger messenger(fMainWindow);
		BMessage message(NEW_FILTER);
		messenger.SendMessage(&message);
	}
}


#pragma mark -- Public Methods --


void
QLApp::SetWindowsFeel(int32 value)
{
	fMainWindow->LockLooper();
	fMainWindow->SetFeel(value ?
		B_MODAL_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
	fMainWindow->UnlockLooper();
}


#pragma mark -- Private Methods --


void
QLApp::_AddToDeskbar()
{
	BDeskbar deskbar;
	entry_ref entry;

	if (!deskbar.HasItem("QuickLaunch")
		&& (be_roster->FindApp(kApplicationSignature, &entry) == B_OK)) {
		status_t err = deskbar.AddItem(&entry);
		if (err != B_OK) {
			status_t err = deskbar.AddItem(new DeskButton(BRect(0, 0, 15, 15),
				&entry, entry.name));
			if (err != B_OK)
				printf("QuickLaunch: Can't install icon in Deskbar\n");
		}
	}
}


void
QLApp::_RemoveFromDeskbar()
{
	BDeskbar deskbar;
	int32 found_id;

	if (deskbar.GetItemInfo("QuickLaunch", &found_id) == B_OK) {
		status_t err = deskbar.RemoveItem(found_id);
		if (err != B_OK) {
			printf("QuickLaunch: Error removing replicant id "
				"%" B_PRId32 ": %s\n", found_id, strerror(err));
		}
	}
}


#pragma mark -- main --


int
main()
{
	QLApp app;
	app.Run();
	return 0;
}
