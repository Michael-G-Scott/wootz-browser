// Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')

/**
 * @param {string} genPath The path to the generated files in the build output.
 * @returns A map of path aliases
 */
module.exports = function (genPath) {
  return {
    // Find files in the current build configurations /gen directory
    'gen': genPath,
    // Generated resources at this path are available at chrome://resources and
    // whilst webpack will still bundle, we keep the alias to the served path
    // to minimize knowledge of specific gen/ paths and easily allow us to not
    // bundle
    // them in the future for certain build configuration, just like chromium.
    'chrome://resources/chrome/fonts': path.join(
      genPath,
      'chrome/ui/webui/resources/fonts'),
    'chrome://resources/chrome': path.join(
      genPath,
      'chrome/ui/webui/resources/tsc'),
    'chrome://resources': path.join(
      genPath, 'ui/webui/resources/tsc'),
    'styled-components': path.resolve(
      __dirname, '../../node_modules/styled-components'),
    // More helpful path for local web-components
    '$web-components': path.resolve(__dirname, '../web-components'),
    '$web-common': path.resolve(__dirname, '../common'),
    // react-markdown uses this deep in its tree, and the browser variant uses innerHTML, conflicting with WebUIs that requires Trusted Types
    // We redirect to an alternative implementation that uses a lookup table to decode named chars instead of innerHTML
    'decode-named-character-reference': path.resolve(__dirname, '../../node_modules/decode-named-character-reference/index.js'),
  }
}
