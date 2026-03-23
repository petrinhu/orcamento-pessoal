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
    sleep(5)
    click("1306052486290.png")
    status=find("YStatus.png")
    doubleClick(status)
    wait(1)
    doubleClick(status.nearby(400).find(Pattern("Status-1.png").similar(0.81)))
    click("1306052650498.png")
    click("1306052703527.png")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    click("1306053834224.png")
    click("1306053853927.png")
    click("1306053869838.png")
    click("1306053885688.png")
    
    click("1306055505239.png")
    r=find("Operations-1.png").right()
    click(r.find("1305987216474.png"))
    
    click("Update.png")
    status2=status.below().find("YStatus.png")
    doubleClick(status2)
    wait(1)
    doubleClick(status2.nearby(400).find(Pattern("Status-1.png").similar(0.81)))
    click("1306052650498.png")
    click("1306052703527.png")
    click("1305986344895.png")
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("search")
    raise
