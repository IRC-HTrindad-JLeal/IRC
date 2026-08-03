#!/bin/bash

printf 'PASS pass\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nPRIVMSG #target :hello\r\n' | nc -w 1 127.0.0.1 6667
