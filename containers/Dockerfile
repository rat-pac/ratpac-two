FROM ratpac/ratpac-two:latest-base
LABEL maintainer="James Shen <jierans@sas.upenn.edu>"

SHELL ["/bin/bash", "-c"]

USER ratuser

WORKDIR /ratpac-setup

RUN ./setup.sh --only ratpac -j$(nproc)
RUN cd ratpac && cmake --install build && cd ..

RUN sed -i '1s/^/#!\/bin\/bash\n/' /ratpac-setup/env.sh
RUN printf '\nexec "$@"\n' >> /ratpac-setup/env.sh
RUN chmod +x /ratpac-setup/env.sh

ENTRYPOINT ["/ratpac-setup/env.sh"]
CMD [ "/bin/bash" ]
