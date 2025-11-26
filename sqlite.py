import sqlite3

# Connect to database (creates file if it doesn't exist)
conn = sqlite3.connect('mydatabase.db')
cursor = conn.cursor()

# Create a table
cursor.execute('''
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        email TEXT UNIQUE,
        age INTEGER
    )
''')

# Insert data (ignore if email already exists)
cursor.execute("INSERT OR IGNORE INTO users (name, email, age) VALUES (?, ?, ?)",
               ("Alice", "alice@example.com", 30))

# Insert multiple rows (ignore duplicates by email)
users = [("Bob", "bob@example.com", 25), ("Charlie", "charlie@example.com", 35)]
cursor.executemany("INSERT OR IGNORE INTO users (name, email, age) VALUES (?, ?, ?)", users)

# Query data
cursor.execute("SELECT * FROM users WHERE age > ?", (25,))
results = cursor.fetchall()
for row in results:
    print(row)

# Commit changes and close
conn.commit()
conn.close()
