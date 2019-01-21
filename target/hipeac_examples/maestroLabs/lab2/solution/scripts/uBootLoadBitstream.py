# (c) Copyright 2014-2019 HIPPEROS S.A.
# (c) Copyright 2010-2013 Universite Libre de Bruxelles
# (c) Copyright 2006-2013 MangoGem S.A.
#
# The license and distribution terms for this file may be
# found in the file LICENSE.txt in this distribution.

"""
Hooks for FPGA load before RTOS boot.
"""

from hipperos.ocd.uboot import UBootError


def preOSboot(_config, _ocdCtrl, ubootTarget):
    """
    Hook function. Loads bitstream from SD card into FPGA before OS boot.
    """
    tmpAddr = '0x18000000'
    filename = 'tulippTutorial.bit.bin'

    if ubootTarget.isPrompt():
        files, _ = ubootTarget.sd_ls()
        if filename not in files:
            return  # TODO temporary, need to ask which toolchain to build option manager

        size = ubootTarget.sd_load(ramAddr=tmpAddr,
                                   filename=filename)
        if ubootTarget.isPrompt():
            ubootTarget.fpga_load(deviceId=0,
                                  ramAddr=tmpAddr,
                                  size=size)
        else:
            raise UBootError('FPGA load failed.')
    else:
        raise UBootError('Load file "{f}" from SD card failed.'
                         ''.format(f=filename))
