#!/usr/bin/env python3
import re
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path
import math

ROOT = Path(__file__).resolve().parent.parent
RENDER_FILE = ROOT / 'include' / 'dagir' / 'render_svg.hpp'
REFERENCE_SVG = ROOT / 'reference.svg'
TEST_SVG = ROOT / 'test.svg'

# Grid to search (kept small to limit rebuilds)
node_w_list = [50.0, 60.0, 70.0]
node_h_list = [30.0, 36.0]
h_gap_list = [16.0, 24.0]
v_gap_list = [24.0, 28.0]
margin_list = [8.0]

# Helpers

def parse_svg_centers(svg_path: Path):
    # Parse <g> elements containing an ellipse or rect and a text label
    tree = ET.parse(svg_path)
    root = tree.getroot()
    # find all groups
    ns = {'svg': 'http://www.w3.org/2000/svg'}
    centers = {}
    # look for ellipse or rect inside groups
    for g in root.findall('.//{http://www.w3.org/2000/svg}g'):
        label = None
        cx = None
        cy = None
        # search for text child
        for text in g.findall('{http://www.w3.org/2000/svg}text'):
            if text.text and text.text.strip():
                label = text.text.strip()
                break
        # ellipse
        ell = g.find('{http://www.w3.org/2000/svg}ellipse')
        if ell is not None:
            cx = float(ell.get('cx'))
            cy = float(ell.get('cy'))
        else:
            # rect -> compute center from x,y,width,height
            rect = g.find('{http://www.w3.org/2000/svg}rect')
            if rect is not None:
                x = float(rect.get('x'))
                y = float(rect.get('y'))
                w = float(rect.get('width'))
                h = float(rect.get('height'))
                cx = x + w/2.0
                cy = y + h/2.0
        if label and cx is not None and cy is not None:
            centers[label] = (cx, cy)
    return centers


def patch_render_file(node_w, node_h, h_gap, v_gap, margin):
    s = RENDER_FILE.read_text(encoding='utf-8')
    s_new = re.sub(r"const double node_w = [0-9.\.]+;",
                   f"const double node_w = {node_w};", s)
    s_new = re.sub(r"const double node_h = [0-9.\.]+;",
                   f"const double node_h = {node_h};", s_new)
    s_new = re.sub(r"const double h_gap = [0-9.\.]+;",
                   f"const double h_gap = {h_gap};", s_new)
    s_new = re.sub(r"const double v_gap = [0-9.\.]+;",
                   f"const double v_gap = {v_gap};", s_new)
    s_new = re.sub(r"const double margin = [0-9.\.]+;",
                   f"const double margin = {margin};", s_new)
    if s_new == s:
        print('No changes applied (pattern mismatch)')
    else:
        RENDER_FILE.write_text(s_new, encoding='utf-8')


def build_project():
    cmd = ['cmake', '--build', 'build', '--config', 'Release', '-j', '4']
    print('Building...')
    subprocess.check_call(cmd, cwd=str(ROOT))


def generate_svg():
    exe = ROOT / 'build' / 'Release' / 'expression2tree.exe'
    cmd = [str(exe), str(ROOT / 'tests' / 'regression_tests' / 'expressions' / 'deep_all_ops.expr'), 'svg']
    print('Generating SVG...')
    with open(TEST_SVG, 'wb') as out:
        subprocess.check_call(cmd, cwd=str(ROOT), stdout=out)


def rmse(a, b):
    s = 0.0
    n = 0
    for k in a:
        if k in b:
            dx = a[k][0] - b[k][0]
            dy = a[k][1] - b[k][1]
            s += dx*dx + dy*dy
            n += 1
    if n == 0:
        return float('inf')
    return math.sqrt(s / n)


def main():
    ref = parse_svg_centers(REFERENCE_SVG)
    print('Reference nodes:', sorted(ref.keys()))
    best = None
    best_params = None
    combination = 0
    total = len(node_w_list)*len(node_h_list)*len(h_gap_list)*len(v_gap_list)*len(margin_list)
    for nw in node_w_list:
        for nh in node_h_list:
            for hg in h_gap_list:
                for vg in v_gap_list:
                    for mg in margin_list:
                        combination += 1
                        print(f'[{combination}/{total}] Trying node_w={nw}, node_h={nh}, h_gap={hg}, v_gap={vg}, margin={mg}')
                        patch_render_file(nw, nh, hg, vg, mg)
                        try:
                            build_project()
                            generate_svg()
                            test = parse_svg_centers(TEST_SVG)
                            score = rmse(ref, test)
                            print('Score RMSE:', score)
                            if best is None or score < best:
                                best = score
                                best_params = (nw, nh, hg, vg, mg)
                                print('New best!', best_params, best)
                        except subprocess.CalledProcessError as e:
                            print('Build or generation failed:', e)
    print('Best params:', best_params, 'rmse=', best)

if __name__ == '__main__':
    main()
