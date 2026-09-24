#!/usr/bin/env python3
"""Read local Docker/host metrics and emit PRTG XML. Requires read access to Docker."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

def collect(root):
    info = json.loads(subprocess.check_output(['docker', 'info', '--format', '{{json .}}'], timeout=15, stderr=subprocess.DEVNULL))
    memory = {line.split(':')[0]: int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines()}
    total, available = memory['MemTotal'], memory['MemAvailable']
    if total <= 0 or not info.get('DockerRootDir'):
        raise ValueError('Missing metrics')
    def disk(path):
        stat = os.statvfs(path)
        if not stat.f_blocks: raise ValueError('Missing filesystem size')
        return round((stat.f_blocks - stat.f_bfree) / stat.f_blocks * 100, 1)
    return [('Docker Engine', 1, 'Count'), ('RAM Usage', round((total-available)/total*100, 1), 'Percent'),
            ('Root Disk Usage', disk(root), 'Percent'), ('Docker Disk Usage', disk(info['DockerRootDir']), 'Percent'),
            ('Containers Running', int(info['ContainersRunning']), 'Count'), ('Containers Total', int(info['Containers']), 'Count')]

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',default='/');args=parser.parse_args()
    doc=ET.Element('prtg')
    try:
        for channel,value,unit in collect(args.root):
            result=ET.SubElement(doc,'result')
            for name,text in [('channel',channel),('value',value),('unit',unit),('float',1 if unit=='Percent' else 0)]:ET.SubElement(result,name).text=str(text)
        ET.SubElement(doc,'text').text='Docker/host metrics available'
    except Exception:
        # Missing measurements are an error, never synthetic zero-percent success.
        ET.SubElement(doc,'error').text='1';ET.SubElement(doc,'text').text='Docker/host metrics unavailable; check local permissions and configuration'
    print(ET.tostring(doc,encoding='unicode'))

if __name__=='__main__':main()
