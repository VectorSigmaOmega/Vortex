data = []
print("Starting memory consumption...")
try:
    while True:
        data.append(' ' * (1024 * 1024))
        print(f"Current size: {len(data)} MB")
except Exception as e:
    print(f"Failed: {e}")
