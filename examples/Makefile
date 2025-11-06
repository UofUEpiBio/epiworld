# Segfaulting (#116):
# 	04-advanced-usage \

PACKAGES := \
	00-hello-world \
	01-seir 01-sir 01-sis \
	02-sir_multiple_runs 02b-sir_multiple_runs \
	03-simple-sir \
	05-user-data \
	07-surveillance \
	08-queuing \
	10-likelihood-free-mcmc \
	11-entities \
	13-genint \
	14-community-hosp
	
# Only with OpenMP
ifneq ($(filter 1 yes true,$(WITH_OPENMP)),)
    PACKAGES += 06-sir-omp 06b-sir-omp
endif
