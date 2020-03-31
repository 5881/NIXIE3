#!/bin/bash
sensors|grep CPU:|awk '{print $2}'|sed 's/+/00/'
