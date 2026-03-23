#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
# works on all platforms
import os

# get the directory containing your running .sikuli
myPath = os.path.dirname(getBundlePath())
if not myPath in sys.path: sys.path.append(myPath)
import shared
try:
    setAutoWaitTimeout(10)
    
    shared.initAllPlugins()
    
    shared.openAllPages()
    
    shared.openSettings()
    
    click("File-1.png")
    click("PrintPrevie-1.png")
    wait(1)
    type(Key.F4, KEY_ALT)
    
    click("File-1.png")
    click("Print-1.png")
    click("Cancel.png")
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("print")
    raise
