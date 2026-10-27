#!/usr/bin/env bash
# Runs commands in, or publishes, the ET: Legacy build images.
#
# Images are tagged with the git blob hash of their Dockerfile, so a tag always
# matches the Dockerfile it was built from. Images missing from the
# registry are built locally, which lets pull requests test Dockerfile changes
# without publishing anything. Publishing also points latest at the image of
# the Dockerfile on master, including after reverts.
#
# Usage: misc/docker-ci.sh run <image> <command...>
#        misc/docker-ci.sh publish <image>
#
# <image> is one of lnx-build, lnx-aarch64-build or android-build.
#
# Commands run as the host user unless DOCKER_USER=root is set, for images
# that install tools into root owned locations during the build. The checkout
# is handed back to the host user afterwards, so later steps can modify it.
set -euo pipefail

action=$1
name=$2
shift 2

case $name in
lnx-build) dockerfile=misc/docker/build.Dockerfile ;;
*) dockerfile=misc/docker/${name%-build}.Dockerfile ;;
esac
# Hash the Dockerfile on disk, so uncommitted changes get their own image too.
blob=$(git hash-object "$dockerfile")
image="etlegacy/$name:${blob:0:12}"

case $action in
run)
    # Prefer a local or published image and only build it as a last resort.
    docker image inspect "$image" >/dev/null 2>&1 ||
        docker pull --quiet "$image" ||
        docker build misc/docker --file "$dockerfile" --tag "$image"

    # Match host ownership and paths so artifacts and warning logs remain
    # usable, and give the container user a writable home without mounting
    # the host one. Variables that are unset on the host stay unset.
    status=0
    docker run --rm \
        --user "${DOCKER_USER:-$(id -u):$(id -g)}" --env HOME=/tmp \
        --env CI --env CI_ETL_DESCRIBE --env CI_ETL_TAG \
        --env SIGNING_KEY_ALIAS --env SIGNING_KEY_PASSWORD --env SIGNING_STORE_PASSWORD \
        --volume "$PWD:$PWD" --workdir "$PWD" \
        "$image" "$@" || status=$?

    if [ "${DOCKER_USER:-}" = root ]; then
        docker run --rm --volume "$PWD:$PWD" "$image" chown -R "$(id -u):$(id -g)" "$PWD"
    fi
    exit "$status"
    ;;
publish)
    if docker manifest inspect "$image" >/dev/null 2>&1; then
        echo "$image is already published."
    else
        docker build misc/docker --file "$dockerfile" --tag "$image"
        docker push "$image"
    fi

    # CI itself only uses the content tags, latest is a convenience for
    # everyone else. Only the Dockerfile currently on master may move it, so
    # re-runs of older master runs leave it alone. Comparing contents rather
    # than commits lets runs of unrelated master commits through.
    latest="etlegacy/$name:latest"
    # Only shallow checkouts, like CI's, may stay shallow. A full clone would
    # otherwise be turned into a shallow one.
    depth=()
    if [ "$(git rev-parse --is-shallow-repository)" = true ]; then
        depth=(--depth=1)
    fi
    git fetch --quiet --no-tags "${depth[@]}" origin refs/heads/master
    if [ "$(git rev-parse --verify --quiet "FETCH_HEAD:$dockerfile" || true)" != "$blob" ]; then
        echo "$dockerfile differs on master, leaving $latest unchanged."
        exit 0
    fi

    # Repin latest inside the registry, which keeps the digest of the content
    # tag without pulling or rebuilding, and also covers reverts to an image
    # that is already published. Raw manifests are identical exactly when
    # both tags point at the same image, so an up to date latest is not
    # rewritten and its history only shows real changes. A missing latest
    # prints nothing and so counts as different.
    current=$(docker buildx imagetools inspect --raw "$latest" 2>/dev/null || true)
    if [ "$current" = "$(docker buildx imagetools inspect --raw "$image")" ]; then
        echo "$latest already points to $image."
    else
        docker buildx imagetools create --tag "$latest" "$image"
    fi
    ;;
*)
    echo "Unknown action: $action" >&2
    exit 1
    ;;
esac
