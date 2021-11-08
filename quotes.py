def main():
    with open("quotes-raw", "r") as f:
        raw_quotes = f.read().splitlines()

    quotes = []
    for line in raw_quotes:

        if not line:
            continue

        if len(line) < 30:
            print(f"Skipping line {line} as it's too short")
            continue

        # remove multiple whitespaces
        while "  " in line:
            line = line.replace("  ", " ")

        # remove quotations marks
        to_remove = ["\"", "“", "”"]
        for r in to_remove:
            line = line.replace(r, "")

        # separate author from quote
        line = line.replace(" -", "@~")

        # strip line
        line = line.strip()

        # append line to list of quotes
        quotes.append(line)

    # remove duplicates
    quotes = list(set(quotes))
    # sort list
    quotes.sort()

    with open(".QUOTES", "w") as f:
        for line in quotes:
            f.write(f"{line}\n")


if __name__ == "__main__":
    main()
