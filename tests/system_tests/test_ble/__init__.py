from settings import settings
from base import Base
import logging
import os
import hashlib
import subprocess
from nose.plugins.attrib import attr

log = logging.getLogger(__name__)

@attr(test_suite=['BLE-ST',
                  'BLE-FT'])
def setup_package():
    if Base.run_ble_s110_setup == True:
        log.debug("BLE S110 tests start")
        Base.dut0.recover()
        Base.dut0.set_softdevice(settings['BLE_S110_SOFTDEVICE_PATH'])
        Base.dut0.flash_softdevice()

def teardown_package():
    if Base.run_ble_s110_setup == True:
        log.debug("BLE S110 tests end")
