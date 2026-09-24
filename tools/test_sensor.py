import contextlib
import importlib.util
import io
import json
from pathlib import Path
import types
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET

spec = importlib.util.spec_from_file_location('docker_sensor', Path(__file__).parents[1] / 'PRTG Sensoren/docker-host.py')
sensor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sensor)

class SensorTests(unittest.TestCase):
    def test_host_metrics_use_actual_values(self):
        with patch.object(sensor.subprocess, 'check_output', return_value=json.dumps(dict(DockerRootDir='/var/lib/docker', ContainersRunning=2, Containers=3)).encode()), \
             patch.object(sensor.Path, 'read_text', return_value='MemTotal: 1000 kB\nMemAvailable: 250 kB\n'), \
             patch.object(sensor.os, 'statvfs', create=True, return_value=types.SimpleNamespace(f_blocks=100, f_bfree=40)):
            values = {k:v for k,v,u in sensor.collect('/')}
        self.assertEqual(values['RAM Usage'],75)
        self.assertEqual(values['Root Disk Usage'],60)
        self.assertEqual(values['Containers Running'],2)

    def test_failure_does_not_emit_healthy_zeroes_or_error_details(self):
        output=io.StringIO()
        with patch('sys.argv',['docker-host.py']), patch.object(sensor,'collect',side_effect=RuntimeError('private detail')), contextlib.redirect_stdout(output):
            sensor.main()
        xml=ET.fromstring(output.getvalue())
        self.assertEqual(xml.findtext('error'),'1')
        self.assertEqual(xml.findall('result'),[])
        self.assertNotIn('private detail',output.getvalue())

if __name__=='__main__':unittest.main()
