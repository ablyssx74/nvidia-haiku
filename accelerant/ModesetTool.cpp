#include <stdio.h>

#include <map>
#include <vector>

#include <Application.h>
#include <Window.h>
#include <Message.h>
#include <Screen.h>

#include <LayoutBuilder.h>

#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>


class ModesetWindow final: public BWindow {
private:
	struct ModeResolutionKey;
	struct ModeRefreshRateKey;

	enum {
		resolutionMenuMsg = 1,
		refreshRateMenuMsg,
		colorSpaceMenuMsg,
		applyMsg,
	};

	struct {
		BMenuField *resolution;
		BMenuField *refreshRate;
		BMenuField *colorSpace;
		BButton *apply;
	} fForm {};

	static uint32 GetRefreshRate(const display_timing &timing);
	static void GetResolutionName(BString &name, const display_timing &timing);
	static void GetRefreshRateName(BString &name, const display_timing &timing);
	static void GetColorSpaceName(BString &name, color_space colorSpace);
	void BuildModeMenus(const display_mode *modes, int32 modeCount);

	void SelectVideoMode(const display_mode &mode);

	void ResolutionSelected(BMessage *msg);
	void ApplyPressed(BMessage *msg);

public:
	ModesetWindow();

	void MessageReceived(BMessage *msg) final;
};


uint32 ModesetWindow::GetRefreshRate(const display_timing &timing)
{
	const bool interlaced = B_TIMING_INTERLACED & timing.flags;
	const uint64 multiplier = interlaced ? 2000000LL : 1000000LL;
	return (multiplier * timing.pixel_clock) / ((uint64)timing.h_total * (uint64)timing.v_total);
}

void ModesetWindow::GetResolutionName(BString &name, const display_timing &timing)
{
	name.SetToFormat("%" B_PRIu16 "x%" B_PRIu16, timing.h_display, timing.v_display);
}

void ModesetWindow::GetRefreshRateName(BString &name, const display_timing &timing)
{
	uint32 refreshRate = GetRefreshRate(timing);
	name.SetToFormat("%" B_PRIu32 ".%03" B_PRIu32 " Hz%s", refreshRate / 1000, refreshRate % 1000, (B_TIMING_INTERLACED & timing.flags) != 0 ? " (interlaced)" : "");
}

void ModesetWindow::GetColorSpaceName(BString &name, color_space colorSpace)
{
	switch (colorSpace) {
		case B_GRAY1:
			name = "GRAY1";
			break;
		case B_GRAY8:
			name = "GRAY8";
			break;
		case B_CMAP8:
			name = "CMAP8";
			break;
		case B_RGB15:
			name = "RGB15";
			break;
		case B_RGB16:
			name = "RGB16";
			break;
		case B_RGB32:
			name = "RGB32";
			break;
		case B_RGB48:
			name = "RGB48";
			break;
		default:
			name = "unknown";
	}
}

struct ModesetWindow::ModeResolutionKey {
	const display_timing &timing;

	bool operator<(const ModeResolutionKey &other) const {
		if (timing.h_display != other.timing.h_display) {
			return timing.h_display < other.timing.h_display;
		}
		return timing.v_display < other.timing.v_display;
	}
};

struct ModesetWindow::ModeRefreshRateKey {
	const display_timing &timing;

	bool operator<(const ModeRefreshRateKey &other) const {
		uint32 refreshRate = ModesetWindow::GetRefreshRate(timing);
		uint32 otherRefreshRate = ModesetWindow::GetRefreshRate(other.timing);
		if (refreshRate != otherRefreshRate) {
			return refreshRate < otherRefreshRate;
		}
		bool interlaced = B_TIMING_INTERLACED & timing.flags;
		bool otherInterlaced = B_TIMING_INTERLACED & other.timing.flags;
		return interlaced < otherInterlaced;
	}
};

void ModesetWindow::BuildModeMenus(const display_mode *modes, int32 modeCount)
{
	// (width, height) -> (refreshRate, interlaced) -> {mode}
	// refreshRates -> modes

	std::map<
		ModeResolutionKey,
		std::map<
			ModeRefreshRateKey,
			std::vector<const display_mode*>
		>
	> groupedModes;

	for (int32 i = 0; i < modeCount; i++) {
		const display_mode &mode = modes[i];

		ModeResolutionKey resolutionKey {mode.timing};
		ModeRefreshRateKey refreshRateKey {mode.timing};

		auto[it1, inserted1] = groupedModes.insert({resolutionKey, {}});
		auto[it2, inserted2] = it1->second.insert({refreshRateKey, {}});
		it2->second.push_back(&mode);
	}

	for (const auto &pair1: groupedModes) {
		BString name;
		GetResolutionName(name, pair1.first.timing);
		BMessage *resolutionItemMsg = new BMessage(resolutionMenuMsg);
		BMenuItem *resolutionItem = new BMenuItem(name.String(), resolutionItemMsg);
		fForm.resolution->Menu()->AddItem(resolutionItem);
		for (const auto &pair2: pair1.second) {
			BMessage modesMsg;
			for (const auto mode: pair2.second) {
				modesMsg.AddData("modes", B_RAW_TYPE, &mode->timing, sizeof(display_timing));
			}
			resolutionItemMsg->AddMessage("refreshRates", &modesMsg);
		}
	}
}


void ModesetWindow::SelectVideoMode(const display_mode &mode)
{

}


void ModesetWindow::ResolutionSelected(BMessage *msg)
{
	fForm.refreshRate->Menu()->RemoveItems(0, fForm.refreshRate->Menu()->CountItems(), true);
	int32 refreshRateCount;
	msg->GetInfo("refreshRates", NULL, &refreshRateCount);
	for (int32 i = 0; ; i++) {
		BMessage modesMsg;
		if (msg->FindMessage("refreshRates", i, &modesMsg) < B_OK) {
			break;
		}
		const display_timing *timing {};
		modesMsg.FindData("modes", B_RAW_TYPE, 0, (const void**)&timing, NULL);

		BString name;
		GetRefreshRateName(name, *timing);
		BMessage *refreshRateMsg = new BMessage(refreshRateMenuMsg);
		refreshRateMsg->AddData("mode", B_RAW_TYPE, timing, sizeof(display_timing));
		BMenuItem *item = new BMenuItem(name.String(), refreshRateMsg);
		if (i == 0) {
			item->SetMarked(true);
		}
		fForm.refreshRate->Menu()->AddItem(item);
	}
}

void ModesetWindow::ApplyPressed(BMessage *msg)
{
	BMenuItem *refreshRateMenuItem = fForm.refreshRate->Menu()->FindMarked();
	BMenuItem *colorSpaceMenuItem = fForm.colorSpace->Menu()->FindMarked();
	if (refreshRateMenuItem == nullptr || colorSpaceMenuItem == nullptr) {
		return;
	}
	refreshRateMenuItem->Message()->PrintToStream();
	colorSpaceMenuItem->Message()->PrintToStream();

	display_mode mode {};
	const display_timing *data {};
	ssize_t dataSize;
	if (refreshRateMenuItem->Message()->FindData("mode", B_RAW_TYPE, (const void**)&data, &dataSize) < B_OK || dataSize != sizeof(display_timing)) {
		fprintf(stderr, "[!] mode missing\n");
		return;
	}
	uint32 colorSpace;
	if (colorSpaceMenuItem->Message()->FindUInt32("colorSpace", &colorSpace) < B_OK) {
		fprintf(stderr, "[!] mode missing\n");
		return;
	}

	memcpy(&mode.timing, data, sizeof(display_timing));
	mode.space = colorSpace;
	mode.virtual_width = mode.timing.h_display;
	mode.virtual_height = mode.timing.v_display;

	BScreen screen(this);
	if (screen.SetMode(&mode, true) < B_OK) {
		fprintf(stderr, "[!] BScreen::SetMode() failed\n");
		return;
	}
}


ModesetWindow::ModesetWindow():
	BWindow(BRect(), "Modeset", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fForm.resolution = new BMenuField("resolution", "Resolution", new BPopUpMenu("resolution"));
	fForm.refreshRate = new BMenuField("refreshRate", "Refresh Rate", new BPopUpMenu("refreshRate"));
	fForm.colorSpace = new BMenuField("colorSpace", "Colors", new BPopUpMenu("colorSpace"));

	fForm.apply = new BButton("apply", "Apply", new BMessage(applyMsg));

	BScreen screen(this);
	display_mode *modes {};
	uint32 modeCount = 0;
	screen.GetModeList(&modes, &modeCount);

	BuildModeMenus(modes, modeCount);

	free(modes);

	color_space colorSpaces[] = {
		B_GRAY1,
		B_GRAY8,
		B_CMAP8,
		B_RGB15,
		B_RGB16,
		B_RGB32,
		B_RGB48,
	};
	for (color_space colorSpace: colorSpaces) {
		BMessage *message = new BMessage(colorSpaceMenuMsg);
		message->AddUInt32("colorSpace", (uint32)colorSpace);
		BString name;
		GetColorSpaceName(name, colorSpace);
		fForm.colorSpace->Menu()->AddItem(new BMenuItem(name.String(), message));
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_SPACING)
		.Add(fForm.resolution)
		.Add(fForm.refreshRate)
		.Add(fForm.colorSpace)
		.Add(fForm.apply)
	.End();
}

void ModesetWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case resolutionMenuMsg: {
			printf("resolutionMenuMsg\n");
			msg->PrintToStream();
			ResolutionSelected(msg);
			break;
		}
		case refreshRateMenuMsg: {
			printf("refreshRateMenuMsg\n");
			msg->PrintToStream();
			break;
		}
		case applyMsg: {
			printf("applyMsg\n");
			ApplyPressed(msg);
			break;
		}
	}
	BWindow::MessageReceived(msg);
}


int main()
{
	BApplication app("application/x-vnd.Test.ModesetTest2");
	ModesetWindow *wnd = new ModesetWindow();
	wnd->CenterOnScreen();
	wnd->Show();
	app.Run();
	return 0;
}
