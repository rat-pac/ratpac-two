FROM ratpac/ratpac-two:latest-base
LABEL maintainer="James Shen <jierans@sas.upenn.edu>"

SHELL ["/bin/bash", "-c"]

USER ratuser

COPY --chown=ratuser:ratuser . /ratpac-setup/ratpac
WORKDIR /ratpac-setup/ratpac
RUN source /ratpac-setup/env.sh && make -j$(nproc)
WORKDIR /ratpac-setup

RUN sed -i '1s/^/#!\/bin\/bash\n/' /ratpac-setup/env.sh
RUN printf '\nexec "$@"\n' >> /ratpac-setup/env.sh
RUN chmod +x /ratpac-setup/env.sh

ENTRYPOINT ["/ratpac-setup/env.sh"]
CMD [ "/bin/bash" ]
