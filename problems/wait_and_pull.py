#!/usr/bin/env python3
import time
from subprocess import call

def get_current_hour():
    return time.gmtime().tm_hour

latest_hour = get_current_hour() - 1;

while(True):
    new_hour = get_current_hour()
    if new_hour > latest_hour:
        print("pulling...")
        call(["./pull.py"])
        call(["git", "add", "prob*"])
        call(["git", "commit", "-am","🤖 adding latest problems 🤖"])
        call(["git", "pull", "--rebase"])
        call(["git", "push"])

        latest_hour = new_hour
    time.sleep(60)
