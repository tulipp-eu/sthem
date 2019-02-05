##################################################################################
#
#  This file is part of the TULIPP Dynamic Partial Utility
#
#  Copyright 2018 Ahmed Sadek, Technische Universität Dresden, TULIPP EU Project
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
##################################################################################

#!/usr/bin/env python3

import os
from tkinter import *
 
window = Tk()
 
window.title("AutoDPR")
 
# First entry   
lbl1 = Label(window, text="Enter the main module name that will be reconfigured. E.g. subtraction", fg="blue")
lbl1.grid(column=0, columnspan=2, row=0, sticky=W, pady=(20, 1))

lbl2 = Label(window, text="Main Module Name")
lbl2.grid(column=0, row=1, sticky=W)

txt1 = Entry(window,width=50)
txt1.grid(column=1, row=1, sticky=W)

# Second entry
lbl3 = Label(window, text="Enter the reconfiguring module name(s) separated by commas. E.g. addition, multiplication", fg="blue")
lbl3.grid(column=0, columnspan=2, row=2, sticky=W, pady=(20, 1))

lbl4 = Label(window, text="Reconfigurable Modules Names")
lbl4.grid(column=0, row=3, sticky=W)

txt2 = Entry(window,width=50)
txt2.grid(column=1, row=3, sticky=W)

# Third entry
lbl5 = Label(window, text="Enter the full path of the working project. E.g. <Project path>/Release", fg="blue")
lbl5.grid(column=0, columnspan=2, row=4, sticky=W, pady=(20, 1))

lbl6 = Label(window, text="Project path")
lbl6.grid(column=0, row=5, sticky=W)

txt3 = Entry(window,width=50)
txt3.grid(column=1, row=5, sticky=W)

# Free Space
lbl6 = Label(window, text="")
lbl6.grid(column=0, row=6, sticky=W)

#Run command
def clicked():
    #Form file
    f = open('dpr_input.tcl', 'w')
    f.write("Main_Module            \""+txt1.get()+'\"\n')
    f.write("Reconfigurable_Modules \""+txt2.get()+'\"\n')
    f.write("Xilinx_Build_Directory \""+txt3.get()+'\"\n')
    f.close()
    status = os.system('vivado -mode batch -source ./create_dpr.tcl')
    if status == 0:
        text.insert(INSERT, "Running finished successfully \n")
        text.insert(INSERT, "The generated bitstreams exist in \"<Project path>/AutoDPR/pr_bitstreams\" \n")
    else:
        text.insert(INSERT, "Running failed \n")
        text.insert(INSERT, "Check \"vivado.log\" for more details\n")


# Run
btn1 = Button(window, text="Run", command=clicked)
btn1.grid(column=0, row=10, sticky=E)

# Exit
btn2 = Button(window, text="Exit", command=sys.exit)
btn2.grid(column=1, row=10, sticky=W)

# Console label
lbl7 = Label(window, text="Output console:")
lbl7.grid(column=0, columnspan=2, row=11, sticky=W, pady=(20, 1))

#Console
text = Text(window)
text.grid(column= 0, columnspan=2, row=12, sticky=W+E)


window.mainloop()

