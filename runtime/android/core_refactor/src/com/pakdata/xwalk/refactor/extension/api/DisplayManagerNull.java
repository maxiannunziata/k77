// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.pakdata.xwalk.refactor.extension.api;

import android.view.Display;

/**
 * A empty implementation for build version lower than API level 17.
 */
public class DisplayManagerNull extends XWalkDisplayManager {
    private final static Display[] NO_DISPLAYS = {};

    public DisplayManagerNull() {
    }

    @Override
    public Display getDisplay(int displayId) {
        return null;
    }

    @Override
    public Display[] getDisplays() {
        return NO_DISPLAYS;
    }

    @Override
    public Display[] getDisplays(String category) {
        return NO_DISPLAYS;
    }

    @Override
    public Display[] getPresentationDisplays() {
        return NO_DISPLAYS;
    }
}
