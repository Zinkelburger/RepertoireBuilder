#!/bin/bash

# Set the database connection details
DB_NAME="mydatabase"
DB_USER="myuser"
DB_PASS="mypassword"

# Connect to the database and delete all data from all tables
PGPASSWORD=$DB_PASS psql -U $DB_USER -d $DB_NAME -c "
SELECT 'TRUNCATE ' || string_agg(table_name, ', ') || ' CASCADE;' 
FROM information_schema.tables 
WHERE table_schema = 'public';
"