#!/bin/sh

for i in ./OGLES*; do
		echo $i -qaf=2000
		$i -qaf=2000
done
