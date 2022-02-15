FROM aitwatchman/ratpac:base
LABEL maintainer="Morgan Askins <maskins@berkeley.edu>"

SHELL ["/bin/bash", "-c"]

USER watchman

WORKDIR /wmutils
RUN ./update.sh

RUN rm -rf ratpac \
 && ./watchmanInstaller.sh --only ratpac sibyl -j8

RUN sed -i '1i#!/bin/bash' env.sh \
 && echo -e "\nexec \"\$@\"" >> env.sh \
 && chmod +x env.sh

USER watchman

ENTRYPOINT ["/wmutils/env.sh"]
CMD [ "/bin/bash" ]
