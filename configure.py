import sys
import os
from ambuild2 import run

# AMBuild determines the build directory from the current working directory
# when this script is executed. The source path is where this script is located.
parser = run.BuildParser(sourcePath=sys.path[0], api='2.2')
parser.options.add_argument('--hl2sdk-root', type=str, dest='hl2sdk_root', default=None,
                       help='Root search folder for HL2SDKs')
parser.options.add_argument('--sm-path', type=str, dest='sm_path', default=None,
                       help='Path to SourceMod')
parser.options.add_argument('--mms-path', type=str, dest='mms_path', default=None,
                       help='Path to Metamod:Source')

parser.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug',
                       help='Enable debugging symbols')
parser.options.add_argument('--enable-optimize', action='store_const', const='1', dest='opt',
                       help='Enable optimization')
parser.options.add_argument('-s', '--sdks', default='present', dest='sdks',
                       help='Build against specified SDKs; valid args are "none", "all", "present",'
                            ' or comma-delimited list of engine names')
parser.options.add_argument('--targets', type=str, dest='targets', default=None,
                          help="Override the target architecture (use commas to separate multiple targets).")
parser.options.add_argument('--breakpad-upload', action='store_const', const='1', dest='breakpadupload', 
                            help="Allow the upload of breakpad symbols to Accelerator.")
parser.Configure()