FROM ratpac/ratpac-two:base
LABEL maintainer="Morgan Askins <maskins@berkeley.edu>"

SHELL ["/bin/bash", "-c"]

USER ratuser

WORKDIR /ratpac-setup

RUN git clone https://github.com/rat-pac/ratpac-two.git ratpac
RUN source env.sh && cd ratpac && make -j6

#RUN sed -i '1i#!/bin/bash' env.sh \
# && echo -e "\nexec \"\$@\"" >> env.sh \
# && chmod +x env.sh
RUN sed -i '1s/^/#!\/bin\/bash\n/' /ratpac-setup/env.sh
RUN printf '\nexec "$@"\n' >> /ratpac-setup/env.sh

ENTRYPOINT ["/ratpac-setup/env.sh"]
CMD [ "/bin/bash" ]
