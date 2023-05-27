package org.xwalk.core.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("xwalk::tenta")
public class DummyNativeTest {

    @CalledByNative
    public static boolean sFunc(){
        return true;
    }
}
