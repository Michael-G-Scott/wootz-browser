// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './icons.html.js';
import '//resources/cr_elements/cr_collapse/cr_collapse.js';
import '//resources/cr_elements/cr_expand_button/cr_expand_button.js';
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js';
import '//resources/cr_elements/cr_icon/cr_icon.js';
import '//resources/cr_elements/cr_shared_style.css.js';
import '//resources/cr_elements/cr_shared_vars.css.js';
import '//resources/cr_elements/icons_lit.html.js';

import {sanitizeInnerHtml} from '//resources/js/parse_html_subset.js';
import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './request.html.js';
import type {Request} from './suggest_internals.mojom-webui.js';
import {RequestStatus} from './suggest_internals.mojom-webui.js';

// Displays a suggest request and its response.
export class SuggestRequestElement extends PolymerElement {
  static get is() {
    return 'suggest-request';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      request: Object,

      requestDataJson_: {
        type: String,
        computed: `computeRquestDataJson_(request.data)`,
      },

      responseJson_: {
        type: String,
        computed: `computeResponseJson_(request.response)`,
      },
    };
  }

  request: Request;
  private requestDataJson_: string = '';
  private responseJson_: string = '';

  private computeRquestDataJson_(): string {
    try {
      // Try to parse the request body, if any.
      this.request.data['Request-Body'] =
          JSON.parse(this.request.data['Request-Body']);
    } finally {
      // Pretty-print the parsed JSON.
      return JSON.stringify(this.request.data, null, 2);
    }
  }

  private computeResponseJson_(): string {
    try {
      // Remove the magic XSSI guard prefix, if any, to get a valid JSON.
      const validJson = this.request.response.replace(')]}\'', '').trim();
      // Try to parse the valid JSON.
      const parsedJson = JSON.parse(validJson);
      // Pretty-print the parsed JSON.
      return JSON.stringify(parsedJson, null, 2);
    } catch (e) {
      return '';
    }
  }

  private getRequestDataHtml_(): TrustedHTML {
    return sanitizeInnerHtml(this.requestDataJson_);
  }

  private getRequestPath_(): string {
    try {
      const url = new URL(this.request.url.url);
      const queryMatches = url.search.match(/(q|delq)=[^&]*/);
      return url.pathname + '?' + (queryMatches ? queryMatches[0] : '');
    } catch (e) {
      return '';
    }
  }

  private getResponseHtml_(): TrustedHTML {
    return sanitizeInnerHtml(this.responseJson_);
  }

  private getStatusIcon_(): string {
    switch (this.request.status) {
      case RequestStatus.kHardcoded:
        return 'suggest:lock';
      case RequestStatus.kCreated:
        return 'cr:create';
      case RequestStatus.kSent:
        return 'cr:schedule';
      case RequestStatus.kSucceeded:
        return 'cr:check-circle';
      case RequestStatus.kFailed:
        return 'cr:cancel';
      default:
        return '';
    }
  }

  private getStatusTitle_(): string {
    switch (this.request.status) {
      case RequestStatus.kHardcoded:
        return 'hardcoded';
      case RequestStatus.kCreated:
        return 'created';
      case RequestStatus.kSent:
        return 'pending';
      case RequestStatus.kSucceeded:
        const startTimeMs = Number(this.request.startTime.internalValue) / 1000;
        const endTimeMs = Number(this.request.endTime.internalValue) / 1000;
        return `succeeded in ${Math.round(endTimeMs - startTimeMs)} ms`;
      case RequestStatus.kFailed:
        return 'failed';
      default:
        return '';
    }
  }

  private getTimestamp_(): string {
    // The JS Date() is based off of the number of milliseconds since the
    // UNIX epoch (1970-01-01 00::00:00 UTC), while |internalValue| of the
    // base::Time (represented in mojom.Time) represents the number of
    // microseconds since the Windows FILETIME epoch (1601-01-01 00:00:00 UTC).
    // This computes the final JS time by computing the epoch delta and the
    // conversion from microseconds to milliseconds.
    const windowsEpoch = Date.UTC(1601, 0, 1, 0, 0, 0, 0);
    const unixEpoch = Date.UTC(1970, 0, 1, 0, 0, 0, 0);
    // |epochDeltaMs| is equivalent to base::Time::kTimeTToMicrosecondsOffset.
    const epochDeltaMs = unixEpoch - windowsEpoch;
    const startTimeMs = Number(this.request.startTime.internalValue) / 1000;
    return (new Date(startTimeMs - epochDeltaMs)).toLocaleTimeString();
  }

  private onCopyRequestClick_() {
    navigator.clipboard.writeText(this.requestDataJson_);

    this.dispatchEvent(new CustomEvent('show-toast', {
      bubbles: true,
      composed: true,
      detail: 'Request Copied to Clipboard',
    }));
  }

  private onCopyResponseClick_() {
    navigator.clipboard.writeText(this.responseJson_);

    this.dispatchEvent(new CustomEvent('show-toast', {
      bubbles: true,
      composed: true,
      detail: 'Response Copied to Clipboard',
    }));
  }

  private onHardcodeResponseClick_() {
    this.dispatchEvent(new CustomEvent('open-hardcode-response-dialog', {
      bubbles: true,
      composed: true,
      detail: this.responseJson_,
    }));
  }

  private onViewRequestClick_() {
    this.dispatchEvent(new CustomEvent('open-view-request-dialog', {
      bubbles: true,
      composed: true,
    }));
  }

  private onViewResponseClick_() {
    this.dispatchEvent(new CustomEvent('open-view-response-dialog', {
      bubbles: true,
      composed: true,
    }));
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'suggest-request': SuggestRequestElement;
  }
}

customElements.define(SuggestRequestElement.is, SuggestRequestElement);
