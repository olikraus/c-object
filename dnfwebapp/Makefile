# dnfwebapp Makefile

ARCH ?= $(shell uname -m)

ifneq ($(filter arm% aarch64,$(ARCH)),)
# ARM: NEON SIMD (enabled by default, no flag required)
SIMD_CFLAGS =
else ifneq ($(filter x86_64 amd64 %86,$(ARCH)),)
# Intel / AMD: SSE 4.2
SIMD_CFLAGS = -msse4.2
else
SIMD_CFLAGS =
endif

CC = gcc
CFLAGS = -g $(SIMD_CFLAGS) -DCO_USE_ZLIB -DXML_GE=1 -Wall -I../co -I../co/expat
LDFLAGS = -lelf -lm -lz -lpthread

COSRC = ../co/co.c ../co/co_extra.c ../co/co_dnf.c ../co/co_bitset.c
COOBJ = $(COSRC:.c=.o)

COMMON_SRC = dwa.c dwa_json.c dwa_op.c dwa_validate.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

all: cli cgi aws

cli: dnfwebapp

cgi: dnfwebapp.cgi

aws: dnfwebapp.aws

dnfwebapp: $(COOBJ) $(COMMON_OBJ) dwa_cli.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

dnfwebapp.cgi: $(COOBJ) $(COMMON_OBJ) dwa_cgi.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

dnfwebapp.aws: $(COOBJ) $(COMMON_OBJ) dwa_aws_lambda.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	chmod a+rwx $@

%.o: %.c dnfwebapp.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to build objects in the parent directory's co folder
../co/%.o: ../co/%.c
	$(CC) $(CFLAGS) -c $< -o $@

cli-test: cli
	./dnfwebapp ../test/dwa_test_input.json -o tmp_test_res.json
	jq .result tmp_test_res.json > tmp_test_res_filtered.json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json
	diff tmp_test_res_filtered.json tmp_expected_filtered.json
	rm tmp_test_res.json tmp_test_res_filtered.json tmp_expected_filtered.json

cgi-test: cgi
	@echo "Testing CGI with curl..."
	curl -X POST -H "Content-Type: application/json" --data-binary @../test/dwa_test_input.json http://localhost/cgi-bin/dnfwebapp.cgi > tmp_cgi_res.json
	@echo "Comparing results..."
	jq .result tmp_cgi_res.json > tmp_cgi_res_filtered.json
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json
	diff tmp_cgi_res_filtered.json tmp_expected_filtered.json
	rm tmp_cgi_res.json tmp_cgi_res_filtered.json tmp_expected_filtered.json


cgi-install: cgi
	cp dnfwebapp.cgi /usr/lib/cgi-bin/dnfwebapp.cgi
	chmod a+rwx /usr/lib/cgi-bin/dnfwebapp.cgi

aws-install:
	python3 -m venv venv
	./venv/bin/pip install flask
	chmod a+rwx lambda_runtime_emulator.py

aws-test: aws
	@echo "Starting Flask Lambda Emulator..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator.log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./dnfwebapp.aws > aws_runtime.log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @../test/dwa_test_input.json > tmp_aws_res.json; \
	kill $$RUNTIME_PID; \
	kill $$FLASK_PID; \
	echo "Comparing AWS results..."; \
	jq .result tmp_aws_res.json > tmp_aws_res_filtered.json; \
	jq .result ../test/dwa_expected_output.json > tmp_expected_filtered.json; \
	diff tmp_aws_res_filtered.json tmp_expected_filtered.json; \
	rm tmp_aws_res.json tmp_aws_res_filtered.json tmp_expected_filtered.json

aws-perf-test: aws perf-input
	@echo "Starting Flask Lambda Emulator for Performance Test..."
	@fuser -k 8080/tcp || true
	@./venv/bin/python3 lambda_runtime_emulator.py > flask_emulator.log 2>&1 & \
	FLASK_PID=$$!; \
	sleep 2; \
	echo "Starting AWS Lambda Runtime binary..."; \
	AWS_LAMBDA_RUNTIME_API=127.0.0.1:8080 ./dnfwebapp.aws > aws_runtime.log 2>&1 & \
	RUNTIME_PID=$$!; \
	sleep 2; \
	echo "Triggering performance invocation..."; \
	curl -s -X POST "http://localhost:8080/2015-03-31/functions/function/invocations" -d @perf_input.json -o tmp_aws_perf.json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n"; \
	kill $$RUNTIME_PID; \
	kill $$FLASK_PID; \
	echo "Internal Log:"; \
	jq -r .log tmp_aws_perf.json; \
	rm tmp_aws_perf.json

perf-input:
	../dnf -gpsd 2 20 -gdnf 2 2 3 -gic 200000 1 -o perf_input.json

cli-perf-test: cli perf-input
	@echo "--- CLI Performance Test ---"
	@/usr/bin/time -f "Total Time: %es" ./dnfwebapp perf_input.json -o perf_res_cli.json > /dev/null
	@echo "Internal Log:"
	@jq -r .log perf_res_cli.json
	@rm perf_res_cli.json

cgi-perf-test: cgi perf-input
	@echo "--- CGI Performance Test ---"
	@curl -s -X POST -H "Content-Type: application/json" \
		--data-binary @perf_input.json \
		-o perf_res_cgi.json \
		-w "Upload/Connect: %{time_pretransfer}s\nProcessing (TTFB): %{time_starttransfer}s\nTotal Time: %{time_total}s\n" \
		http://localhost/cgi-bin/dnfwebapp.cgi
	@echo "Internal Log:"
	@jq -r .log perf_res_cgi.json
	@rm perf_res_cgi.json

perf-test: cli-perf-test cgi-perf-test
	rm perf_input.json

clean:
	-rm -f dnfwebapp dnfwebapp.cgi dnfwebapp.aws $(COMMON_OBJ) dwa_cli.o dwa_cgi.o dwa_aws_lambda.o
	-rm -f aws_rie.log aws_runtime.log flask_emulator.log out.json perf_input.json tmp_aws_res.json
	-rm -rf venv tmp
