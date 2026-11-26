CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -O2 -pthread
TARGET  = document_analyzer

all: $(TARGET)

$(TARGET): document_analyzer.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET) "IoT" sample_docs/document1.txt sample_docs/document2.txt sample_docs/document3.txt

clean:
	rm -f $(TARGET) *_report.txt
