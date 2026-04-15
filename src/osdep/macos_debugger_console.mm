#include "macos_debugger_console.h"

#if defined(AMIBERRY_MACOS)

#import <AppKit/AppKit.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <dispatch/dispatch.h>
#include <mutex>
#include <string>

#include "debug.h"

constexpr CGFloat kConsoleWidth = 900.0;
constexpr CGFloat kConsoleHeight = 560.0;
constexpr CGFloat kPadding = 12.0;
constexpr CGFloat kInputHeight = 28.0;
constexpr CGFloat kAutoScrollEpsilon = 4.0;
constexpr NSUInteger kMaxTranscriptLines = 1000;
constexpr NSUInteger kMaxInputHistory = 50;

std::mutex debugger_console_mutex;
std::condition_variable debugger_console_cv;
std::deque<std::string> debugger_console_lines;
bool debugger_console_closed = false;

static void clear_pending_input_locked()
{
	debugger_console_lines.clear();
}

static void queue_line(const std::string& line)
{
	{
		std::lock_guard<std::mutex> lock(debugger_console_mutex);
		debugger_console_lines.push_back(line);
		debugger_console_closed = false;
	}
	debugger_console_cv.notify_all();
}

static bool debugger_console_is_closed()
{
	std::lock_guard<std::mutex> lock(debugger_console_mutex);
	return debugger_console_closed;
}

static bool debugger_console_should_exit_wait()
{
	return debugger_console_is_closed() || !debugger_active;
}

static bool try_dequeue_line(char* out, const int maxlen, int* out_len)
{
	std::lock_guard<std::mutex> lock(debugger_console_mutex);
	if (debugger_console_lines.empty()) {
		return false;
	}

	std::string line = std::move(debugger_console_lines.front());
	debugger_console_lines.pop_front();

	const int len = std::min(maxlen - 1, static_cast<int>(line.size()));
	std::memcpy(out, line.c_str(), len);
	out[len] = 0;
	*out_len = len;
	return true;
}

static void run_on_main_sync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

static void pump_debugger_window_events_for_mode(NSRunLoopMode mode)
{
	for (;;) {
		NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
			untilDate:[NSDate distantPast]
			inMode:mode
			dequeue:YES];
		if (!event) {
			break;
		}
		[NSApp sendEvent:event];
	}
}

static void pump_debugger_window_events()
{
	@autoreleasepool {
		pump_debugger_window_events_for_mode(NSDefaultRunLoopMode);
		pump_debugger_window_events_for_mode(NSEventTrackingRunLoopMode);
		[NSApp updateWindows];
		NSDate* until_date = [NSDate dateWithTimeIntervalSinceNow:0.05];
		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:until_date];
		[[NSRunLoop currentRunLoop] runMode:NSEventTrackingRunLoopMode beforeDate:until_date];
	}
}

@interface AmiberryDebuggerWindow : NSWindow
@end

@implementation AmiberryDebuggerWindow

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	if ([event type] != NSEventTypeKeyDown) {
		return [super performKeyEquivalent:event];
	}

	const NSEventModifierFlags modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
	if ((modifiers & NSEventModifierFlagCommand) == 0 || (modifiers & NSEventModifierFlagControl) != 0) {
		return [super performKeyEquivalent:event];
	}

	NSString* key = [[event charactersIgnoringModifiers] lowercaseString];
	SEL action = nullptr;
	if ([key isEqualToString:@"c"]) {
		action = @selector(copy:);
	} else if ([key isEqualToString:@"x"]) {
		action = @selector(cut:);
	} else if ([key isEqualToString:@"v"]) {
		action = @selector(paste:);
	} else if ([key isEqualToString:@"a"]) {
		action = @selector(selectAll:);
	} else if ([key isEqualToString:@"z"]) {
		action = (modifiers & NSEventModifierFlagShift) != 0 ? @selector(redo:) : @selector(undo:);
	}

	if (!action) {
		return [super performKeyEquivalent:event];
	}

	NSResponder* responder = [self firstResponder];
	if (responder && [responder tryToPerform:action with:self]) {
		return YES;
	}
	if ([NSApp sendAction:action to:nil from:self]) {
		return YES;
	}
	return [super performKeyEquivalent:event];
}

@end

@interface AmiberryDebuggerConsoleController : NSObject <NSWindowDelegate, NSTextFieldDelegate>
{
@private
	NSWindow* window_;
	NSScrollView* output_scroll_view_;
	NSTextView* output_view_;
	NSTextField* input_field_;
	NSMutableArray* input_history_;
	NSString* history_draft_;
	NSInteger history_index_;
}
- (void)showWindow;
- (void)hideWindowProgrammatically:(BOOL)programmatic;
- (BOOL)isVisible;
- (void)appendText:(NSString*)text;
- (void)clearTranscript;
- (void)resetHistoryNavigation;
- (NSDictionary*)outputTextAttributes;
- (BOOL)isTranscriptPinnedToBottom;
- (void)scrollTranscriptToBottom;
@end

static AmiberryDebuggerConsoleController* debugger_console_controller = nil;

static AmiberryDebuggerConsoleController* ensure_controller()
{
	if (!debugger_console_controller) {
		debugger_console_controller = [[AmiberryDebuggerConsoleController alloc] init];
	}
	return debugger_console_controller;
}

@implementation AmiberryDebuggerConsoleController

- (instancetype)init
{
	self = [super init];
	if (!self) {
		return nil;
	}

	const NSRect frame = NSMakeRect(0.0, 0.0, kConsoleWidth, kConsoleHeight);
	window_ = [[AmiberryDebuggerWindow alloc] initWithContentRect:frame
		styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
			NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
		backing:NSBackingStoreBuffered
		defer:NO];
	[window_ center];
	[window_ setTitle:@"Amiberry Debugger"];
	[window_ setReleasedWhenClosed:NO];
	[window_ setDelegate:self];
	[window_ setMinSize:NSMakeSize(640.0, 360.0)];
	[window_ setBackgroundColor:[NSColor windowBackgroundColor]];

	NSView* content_view = [window_ contentView];
	const NSRect bounds = [content_view bounds];

	const NSRect input_frame = NSMakeRect(
		kPadding,
		kPadding,
		bounds.size.width - (kPadding * 2.0),
		kInputHeight);
	input_field_ = [[NSTextField alloc] initWithFrame:input_frame];
	[input_field_ setAutoresizingMask:NSViewWidthSizable | NSViewMaxYMargin];
	[input_field_ setDelegate:self];
	[input_field_ setTarget:self];
	[input_field_ setAction:@selector(submitInput:)];
	[input_field_ setPlaceholderString:@"Enter debugger command and press Return"];
	NSFont* mono_font = [NSFont fontWithName:@"Menlo" size:12.0];
	if (!mono_font) {
		mono_font = [NSFont userFixedPitchFontOfSize:12.0];
	}
	if (mono_font) {
		[input_field_ setFont:mono_font];
	}
	[input_field_ setTextColor:[NSColor textColor]];
	[input_field_ setBackgroundColor:[NSColor textBackgroundColor]];
	[content_view addSubview:input_field_];

	const NSRect scroll_frame = NSMakeRect(
		kPadding,
		kPadding * 2.0 + kInputHeight,
		bounds.size.width - (kPadding * 2.0),
		bounds.size.height - (kPadding * 3.0) - kInputHeight);
	NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:scroll_frame];
	[scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[scroll_view setHasVerticalScroller:YES];
	[scroll_view setHasHorizontalScroller:NO];
	[scroll_view setBorderType:NSBezelBorder];
	[scroll_view setDrawsBackground:YES];
	[scroll_view setBackgroundColor:[NSColor textBackgroundColor]];
	output_scroll_view_ = scroll_view;

	output_view_ = [[NSTextView alloc] initWithFrame:scroll_frame];
	[output_view_ setEditable:NO];
	[output_view_ setSelectable:YES];
	[output_view_ setVerticallyResizable:YES];
	[output_view_ setHorizontallyResizable:NO];
	[output_view_ setAutoresizingMask:NSViewWidthSizable];
	[output_view_ setMinSize:NSMakeSize(0.0, scroll_frame.size.height)];
	[output_view_ setMaxSize:NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX)];
	[output_view_ setRichText:NO];
	[output_view_ setImportsGraphics:NO];
	[output_view_ setUsesFontPanel:NO];
	[output_view_ setUsesFindPanel:YES];
	[output_view_ setTextColor:[NSColor textColor]];
	[output_view_ setBackgroundColor:[NSColor textBackgroundColor]];
	[output_view_ setDrawsBackground:YES];
	[output_view_ setTextContainerInset:NSMakeSize(6.0, 6.0)];
	[[output_view_ textContainer] setWidthTracksTextView:YES];
	[[output_view_ textContainer] setContainerSize:NSMakeSize(scroll_frame.size.width, CGFLOAT_MAX)];
	if (mono_font) {
		[output_view_ setFont:mono_font];
	}

	[scroll_view setDocumentView:output_view_];
	[content_view addSubview:scroll_view];
	[scroll_view release];

	input_history_ = [[NSMutableArray alloc] init];
	history_draft_ = nil;
	history_index_ = -1;

	return self;
}

- (NSDictionary*)outputTextAttributes
{
	NSFont* font = [output_view_ font];
	if (!font) {
		font = [NSFont userFixedPitchFontOfSize:12.0];
	}
	return @{
		NSForegroundColorAttributeName: [NSColor textColor],
		NSFontAttributeName: font
	};
}

- (BOOL)isTranscriptPinnedToBottom
{
	if (!output_scroll_view_ || !output_view_) {
		return YES;
	}

	NSClipView* clip_view = [output_scroll_view_ contentView];
	if (!clip_view) {
		return YES;
	}

	const NSRect visible_rect = [clip_view documentVisibleRect];
	const CGFloat max_visible_y = NSMaxY(visible_rect);
	const CGFloat max_document_y = NSMaxY([output_view_ frame]);
	return max_document_y - max_visible_y <= kAutoScrollEpsilon;
}

- (void)scrollTranscriptToBottom
{
	NSTextStorage* storage = [output_view_ textStorage];
	if (!storage) {
		return;
	}

	const NSRange end_range = NSMakeRange([storage length], 0);
	[output_view_ scrollRangeToVisible:end_range];
}

- (void)restoreInput:(NSString*)value
{
	NSString* text = value ? value : @"";
	[input_field_ setStringValue:text];
	[window_ makeFirstResponder:input_field_];
	NSText* editor = [window_ fieldEditor:YES forObject:input_field_];
	if (editor) {
		[editor setSelectedRange:NSMakeRange([text length], 0)];
	}
}

- (void)resetHistoryNavigation
{
	history_index_ = -1;
	[history_draft_ release];
	history_draft_ = nil;
}

- (void)addHistoryEntry:(NSString*)command
{
	if (!command || [command length] == 0) {
		[self resetHistoryNavigation];
		return;
	}
	if ([input_history_ count] > 0 && [[input_history_ lastObject] isEqualToString:command]) {
		[self resetHistoryNavigation];
		return;
	}
	if ([input_history_ count] >= kMaxInputHistory) {
		[input_history_ removeObjectAtIndex:0];
	}
	[input_history_ addObject:[NSString stringWithString:command]];
	[self resetHistoryNavigation];
}

- (void)showPreviousHistoryEntry
{
	if ([input_history_ count] == 0) {
		return;
	}
	if (history_index_ < 0) {
		[history_draft_ release];
		history_draft_ = [[input_field_ stringValue] copy];
		history_index_ = static_cast<NSInteger>([input_history_ count]) - 1;
	} else if (history_index_ > 0) {
		history_index_--;
	}
	[self restoreInput:[input_history_ objectAtIndex:static_cast<NSUInteger>(history_index_)]];
}

- (void)showNextHistoryEntry
{
	if (history_index_ < 0) {
		return;
	}
	if (static_cast<NSUInteger>(history_index_ + 1) < [input_history_ count]) {
		history_index_++;
		[self restoreInput:[input_history_ objectAtIndex:static_cast<NSUInteger>(history_index_)]];
		return;
	}
	NSString* draft = history_draft_;
	[self restoreInput:draft];
	[self resetHistoryNavigation];
}

- (void)trimTranscriptIfNeeded
{
	NSString* contents = [output_view_ string];
	if (!contents || [contents length] == 0) {
		return;
	}

	__block NSUInteger total_lines = 0;
	[contents enumerateSubstringsInRange:NSMakeRange(0, [contents length])
		options:NSStringEnumerationByLines | NSStringEnumerationSubstringNotRequired
		usingBlock:^(NSString* substring, NSRange substringRange, NSRange enclosingRange, BOOL* stop) {
			total_lines++;
		}];
	if (total_lines <= kMaxTranscriptLines) {
		return;
	}

	__block NSUInteger lines_to_trim = total_lines - kMaxTranscriptLines;
	__block NSUInteger trim_index = 0;
	[contents enumerateSubstringsInRange:NSMakeRange(0, [contents length])
		options:NSStringEnumerationByLines | NSStringEnumerationSubstringNotRequired
		usingBlock:^(NSString* substring, NSRange substringRange, NSRange enclosingRange, BOOL* stop) {
			if (lines_to_trim == 0) {
				*stop = YES;
				return;
			}
			lines_to_trim--;
			trim_index = NSMaxRange(enclosingRange);
			if (lines_to_trim == 0) {
				*stop = YES;
			}
		}];
	if (trim_index > 0) {
		[[output_view_ textStorage] replaceCharactersInRange:NSMakeRange(0, trim_index) withString:@""];
	}
}

- (void)submitInput:(id)sender
{
	NSString* value = [input_field_ stringValue];
	[self addHistoryEntry:value];
	const char* utf8 = value ? [value UTF8String] : nullptr;
	const std::string line = utf8 ? std::string(utf8) : std::string();
	queue_line(line);

	if (value) {
		[self appendText:[value stringByAppendingString:@"\n"]];
	}
	[input_field_ setStringValue:@""];
}

- (BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)command_selector
{
	if (command_selector == @selector(insertNewline:)) {
		[self submitInput:control];
		return YES;
	}
	if (command_selector == @selector(moveUp:)) {
		[self showPreviousHistoryEntry];
		return YES;
	}
	if (command_selector == @selector(moveDown:)) {
		[self showNextHistoryEntry];
		return YES;
	}
	return NO;
}

- (BOOL)windowShouldClose:(id)sender
{
	[self hideWindowProgrammatically:NO];
	return NO;
}

- (void)showWindow
{
	if (![window_ isVisible]) {
		[self clearTranscript];
		[input_field_ setStringValue:@""];
	}
	[self resetHistoryNavigation];
	[NSApp activateIgnoringOtherApps:YES];
	[window_ makeKeyAndOrderFront:nil];
	[window_ orderFrontRegardless];
	[window_ makeFirstResponder:input_field_];
}

- (void)hideWindowProgrammatically:(BOOL)programmatic
{
	[window_ orderOut:nil];

	if (programmatic) {
		{
			std::lock_guard<std::mutex> lock(debugger_console_mutex);
			clear_pending_input_locked();
			debugger_console_closed = true;
		}
		debugger_console_cv.notify_all();
		return;
	}

	{
		std::lock_guard<std::mutex> lock(debugger_console_mutex);
		clear_pending_input_locked();
	}
	queue_line("x");
}

- (BOOL)isVisible
{
	return [window_ isVisible];
}

- (void)appendText:(NSString*)text
{
	if (!text || [text length] == 0) {
		return;
	}

	NSTextStorage* storage = [output_view_ textStorage];
	if (!storage) {
		return;
	}

	const bool should_autoscroll = [self isTranscriptPinnedToBottom];
	NSAttributedString* attributed = [[NSAttributedString alloc] initWithString:text attributes:[self outputTextAttributes]];
	[storage appendAttributedString:attributed];
	[attributed release];
	[self trimTranscriptIfNeeded];
	if (should_autoscroll) {
		[self scrollTranscriptToBottom];
	}
}

- (void)clearTranscript
{
	NSAttributedString* empty = [[NSAttributedString alloc] initWithString:@""];
	[[output_view_ textStorage] setAttributedString:empty];
	[empty release];
	[self resetHistoryNavigation];
}

@end

bool macos_debugger_console_supported()
{
	return true;
}

void macos_debugger_console_open()
{
	run_on_main_sync(^{
		AmiberryDebuggerConsoleController* controller = ensure_controller();
		if (![controller isVisible]) {
			std::lock_guard<std::mutex> lock(debugger_console_mutex);
			clear_pending_input_locked();
			debugger_console_closed = false;
		}
		[controller showWindow];
	});
}

void macos_debugger_console_close()
{
	{
		std::lock_guard<std::mutex> lock(debugger_console_mutex);
		clear_pending_input_locked();
		debugger_console_closed = true;
	}
	debugger_console_cv.notify_all();

	run_on_main_sync(^{
		if (debugger_console_controller) {
			[debugger_console_controller hideWindowProgrammatically:YES];
		}
	});
}

void macos_debugger_console_activate()
{
	run_on_main_sync(^{
		if (debugger_console_controller) {
			[debugger_console_controller showWindow];
		}
	});
}

void macos_debugger_console_write(const char* text)
{
	if (!text || !text[0]) {
		return;
	}

	run_on_main_sync(^{
		AmiberryDebuggerConsoleController* controller = ensure_controller();
		NSString* string = [NSString stringWithUTF8String:text];
		if (!string) {
			string = [NSString stringWithCString:text encoding:NSISOLatin1StringEncoding];
		}
		[controller appendText:string];
	});
}

bool macos_debugger_console_has_input()
{
	std::lock_guard<std::mutex> lock(debugger_console_mutex);
	return !debugger_console_lines.empty();
}

char macos_debugger_console_getch()
{
	return 0;
}

int macos_debugger_console_get(char* out, const int maxlen)
{
	if (!out || maxlen <= 0) {
		return -1;
	}

	if ([NSThread isMainThread]) {
		for (;;) {
			int len = 0;
			if (try_dequeue_line(out, maxlen, &len)) {
				return len;
			}
			if (debugger_console_should_exit_wait()) {
				return -1;
			}
			pump_debugger_window_events();
		}
	}

	std::unique_lock<std::mutex> lock(debugger_console_mutex);
	while (debugger_console_lines.empty() && !debugger_console_closed && debugger_active) {
		debugger_console_cv.wait_for(lock, std::chrono::milliseconds(50));
	}

	if (debugger_console_lines.empty()) {
		return -1;
	}

	std::string line = std::move(debugger_console_lines.front());
	debugger_console_lines.pop_front();
	lock.unlock();

	const int len = std::min(maxlen - 1, static_cast<int>(line.size()));
	std::memcpy(out, line.c_str(), len);
	out[len] = 0;
	return len;
}

#endif
