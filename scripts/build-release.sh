#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_root="$repo_dir/build/release"
export PICOTOOL_FETCH_FROM_GIT_PATH="${PICOTOOL_FETCH_FROM_GIT_PATH:-$build_root/picotool}"
mkdir -p "$build_root"
stage=$(mktemp -d "$build_root/package.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT

for board in m1 m2; do
    pico_board=murmulator
    if [[ $board == m2 ]]; then
        pico_board=murmulator2
    fi

    for processor in p1 p2; do
        platform=rp2040
        if [[ $processor == p2 ]]; then
            platform=rp2350
        fi

        for video in vga-hdmi-tft composite soft-composite; do
            variant="$board$processor-$video"
            build_dir="$build_root/$variant"
            cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
                -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD="$pico_board" \
                -DPICO_PLATFORM="$platform" -DMURM_VIDEO="$video"
            cmake --build "$build_dir"

            version=$(sed -n 's/^#define SOFT_VERSION "\(.*\)"$/\1/p' "$build_dir/generated/config.h")
            test -n "$version"
            firmware="murmulator-$variant-v$version.uf2"
            test -s "$build_dir/$firmware"
            cp -- "$build_dir/$firmware" "$stage/"
        done
    done
done

cp -- "$repo_dir/RELEASE_NOTES.md" "$repo_dir"/release/help_murmulator_{eng,rus}.txt "$stage/"
archive="murmulator-v$version.zip"
(
    cd "$stage"
    sha256sum ./*.uf2 > SHA256SUMS.txt
    zip -q "$archive" ./*.uf2 SHA256SUMS.txt RELEASE_NOTES.md help_murmulator_{eng,rus}.txt
)
mv -- "$stage/$archive" "$build_root/$archive"
printf '\n%s\n' "$build_root/$archive"
