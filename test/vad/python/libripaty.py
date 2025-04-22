import csv

output_file = "output.csv"

# Open the CSV file for writing
with open(output_file, mode='w', newline='') as file:
    writer = csv.writer(file)
    
    # Write the header
    writer.writerow(["key_index", "start", "stop"])
    
    # Iterate through the JSON data and write rows to the CSV file
    for key_index, entries in data.items():
        for entry in entries:
            writer.writerow([key_index, entry["start"], entry["stop"]])

print(f"CSV file '{output_file}' has been created.")