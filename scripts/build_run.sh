#!/bin/bash
bash "$(dirname "$0")/build.sh"
bash "$(dirname "$0")/run_kvm.sh"