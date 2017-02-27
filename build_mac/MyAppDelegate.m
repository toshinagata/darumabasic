//
//  MyAppDelegate.m
//  Daruma Basic
//
//  Created by Toshi Nagata on 16/01/09.
//  Copyright 2016 Toshi Nagata. All rights reserved.
//

#import "MyAppDelegate.h"
#include "daruma.h"

@implementation MyAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[window makeKeyAndOrderFront:self];
	bs_runloop();
	[NSApp terminate:self];
}

@end
