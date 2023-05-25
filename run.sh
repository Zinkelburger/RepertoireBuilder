#!/bin/bash
docker run -d --name my-database -p 5432:5432 -v /path/to/data:/var/lib/postgresql/data database-image
