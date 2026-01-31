#!/bin/bash

socat -d -d pty,raw,echo=0,link=./ttySender pty,raw,echo=0,link=./ttyReceiver

