#!/bin/bash

# Read the database volume location from the configuration file
databaseVolumeLocation=$(grep "^databaseVolumeLocation=" configuration.txt | cut -d'=' -f2)

if ! docker start my-database; then
    docker run -d --name my-database -p 5432:5432 -v $databaseVolumeLocation:/var/lib/postgresql/data database-image
fi
