// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying a list of cellular
 * APNs
 */

import '//resources/ash/common/cr_elements/localized_link/localized_link.js';
import './network_shared.css.js';
import '//resources/polymer/v3_0/iron-list/iron-list.js';
import '//resources/ash/common/network/apn_list_item.js';
import '//resources/ash/common/network/apn_detail_dialog.js';
import '//resources/ash/common/network/apn_selection_dialog.js';
import '//resources/ash/common/cr_elements/icons.html.js';

import {assert} from '//resources/ash/common/assert.js';
import {I18nBehavior, I18nBehaviorInterface} from '//resources/ash/common/i18n_behavior.js';
import {ApnDetailDialog} from '//resources/ash/common/network/apn_detail_dialog.js';
import {ApnDetailDialogMode, ApnEventData, isAttachApn, isDefaultApn} from '//resources/ash/common/network/cellular_utils.js';
import {ApnProperties, ApnSource, ApnState, ApnType, ManagedCellularProperties} from '//resources/mojo/chromeos/services/network_config/public/mojom/cros_network_config.mojom-webui.js';
import {PortalState} from '//resources/mojo/chromeos/services/network_config/public/mojom/network_types.mojom-webui.js';
import {afterNextRender, mixinBehaviors, PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './apn_list.html.js';

/* @type {string} */
const SHILL_INVALID_APN_ERROR = 'invalid-apn';

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 */
const ApnListBase = mixinBehaviors([I18nBehavior], PolymerElement);

/** @polymer */
export class ApnList extends ApnListBase {
  static get is() {
    return 'apn-list';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /** The GUID of the network to display details for. */
      guid: String,

      /**@type {!ManagedCellularProperties}*/
      managedCellularProperties: {
        type: Object,
      },

      errorState: String,

      /** @type {?PortalState} */
      portalState: {
        type: Object,
      },

      shouldOmitLinks: {
        type: Boolean,
        value: false,
      },

      shouldDisallowApnModification: {
        type: Boolean,
        value: false,
      },

      /** @private */
      apns_: {
        type: Object,
        value: [],
        computed: 'computeApns_(managedCellularProperties)',
      },

      /** @private */
      hasEnabledDefaultCustomApn_: {
        type: Boolean,
        computed:
            'computeHasEnabledDefaultCustomApn_(managedCellularProperties)',
      },

      /** @private */
      shouldShowApnDetailDialog_: {
        type: Boolean,
        value: false,
      },

      /**
       * The mode in which the apn detail dialog is opened.
       * @type {ApnDetailDialogMode}
       * @private
       */
      apnDetailDialogMode_: {
        type: Object,
        value: ApnDetailDialogMode.CREATE,
      },

      /** @private */
      shouldShowApnSelectionDialog_: {
        type: Boolean,
        value: false,
      },
    };
  }

  /**
   * @param {!Event} e
   * @private
   */
  onZeroStateCreateApnLinkClicked_(e) {
    // A place holder href with the value "#" is used to have a compliant link.
    // This prevents the browser from navigating the window to "#"
    e.detail.event.preventDefault();
    e.stopPropagation();
    this.openApnDetailDialogInCreateMode();
  }

  openApnDetailDialogInCreateMode() {
    this.showApnDetailDialog_(ApnDetailDialogMode.CREATE, /* apn= */ undefined);
  }

  openApnSelectionDialog() {
    this.shouldShowApnSelectionDialog_ = true;
  }

  /**
   * @return {boolean}
   * @private
   */
  shouldShowZeroStateContent_() {
    if (!this.managedCellularProperties) {
      return true;
    }

    if (this.managedCellularProperties.connectedApn) {
      return false;
    }

    // Don't display the zero-state text if there's an APN-related error.
    if (this.errorState === SHILL_INVALID_APN_ERROR) {
      return false;
    }

    return !this.getCustomApnList_().length;
  }

  /**
   * @return {boolean}
   * @private
   */
  shouldShowErrorMessage_() {
    // In some instances, there can be an |errorState| and also a connected APN.
    // Don't show the error message as the network is actually connected.
    if (this.managedCellularProperties &&
        this.managedCellularProperties.connectedApn) {
      return false;
    }

    return this.errorState === SHILL_INVALID_APN_ERROR;
  }

  /**
   * @return {string}
   * @private
   */
  getErrorMessage_() {
    if (!this.managedCellularProperties || !this.errorState) {
      return '';
    }

    if (this.getCustomApnList_().some(apn => apn.state === ApnState.kEnabled)) {
      return this.i18n('apnSettingsCustomApnsErrorMessage');
    }

    return this.i18n('apnSettingsDatabaseApnsErrorMessage');
  }

  /**
   * Returns an array with all the APN properties that need to be displayed.
   * TODO(b/162365553): Handle managedCellularProperties.apnList.policyValue
   * when policies are included.
   * @return {Array<!ApnProperties>}
   * @private
   */
  computeApns_() {
    if (!this.managedCellularProperties) {
      return [];
    }

    const {connectedApn} = this.managedCellularProperties;
    const customApnList = this.getCustomApnList_();

    // Move the connected APN to the front if it exists
    if (connectedApn) {
      const customApnsWithoutConnectedApn =
          customApnList.filter(apn => apn.id !== connectedApn.id);
      return [connectedApn, ...customApnsWithoutConnectedApn];
    }
    return customApnList;
  }

  /**
   * Returns true if the APN on this index is connected.
   * @param {number} index index in the APNs array.
   * @return {boolean}
   * @private
   */
  isApnConnected_(index) {
    return !!this.managedCellularProperties &&
        !!this.managedCellularProperties.connectedApn && index === 0;
  }

  /**
   * Returns true if currentApn is the only enabled default APN and there is
   * at least one enabled attach APN.
   * @param {!ApnProperties} currentApn
   * @return {boolean}
   * @private
   */
  shouldDisallowDisablingRemoving_(currentApn) {
    assert(this.managedCellularProperties);
    if (!currentApn.id) {
      return true;
    }

    const customApnList = this.getCustomApnList_();
    if (!customApnList.some(
            apn => isAttachApn(apn) && !isDefaultApn(apn) &&
                apn.state === ApnState.kEnabled)) {
      return false;
    }

    const defaultEnabledApnList = customApnList.filter(
        apn => isDefaultApn(apn) && apn.state === ApnState.kEnabled);

    return defaultEnabledApnList.length === 1 &&
        currentApn.id === defaultEnabledApnList[0].id;
  }

  /**
   * Returns true if there are no enabled default APNs and the current APN has
   * only an attach APN type.
   * @param {!ApnProperties} currentApn
   * @return {boolean}
   * @private
   */
  shouldDisallowEnabling_(currentApn) {
    assert(this.managedCellularProperties);
    if (!currentApn.id) {
      return true;
    }

    if (this.hasEnabledDefaultCustomApn_) {
      return false;
    }

    return isAttachApn(currentApn) && !isDefaultApn(currentApn);
  }

  /**
   * @param {!Event} event
   * @private
   */
  onShowApnDetailDialog_(event) {
    event.stopPropagation();
    if (this.shouldShowApnDetailDialog_) {
      return;
    }
    const eventData = /** @type {!ApnEventData} */ (event.detail);
    this.showApnDetailDialog_(eventData.mode, eventData.apn);
  }

  /**
   * @param {!ApnDetailDialogMode} mode
   * @param {ApnProperties|undefined} apn
   * @private
   */
  showApnDetailDialog_(mode, apn) {
    this.shouldShowApnDetailDialog_ = true;
    this.apnDetailDialogMode_ = mode;
    // Added to ensure dom-if stamping.
    afterNextRender(this, () => {
      const apnDetailDialog = /** @type {ApnDetailDialog} */ (
          this.shadowRoot.querySelector('#apnDetailDialog'));
      assert(!!apnDetailDialog);
      apnDetailDialog.apnProperties = apn;
    });
  }

  /**
   *
   * @param event {!Event}
   * @private
   */
  onApnDetailDialogClose_(event) {
    this.shouldShowApnDetailDialog_ = false;
  }

  /**
   *
   * @param event {!Event}
   * @private
   */
  onApnSelectionDialogClose_(event) {
    this.shouldShowApnSelectionDialog_ = false;
  }

  /**
   * @returns {Array<ApnProperties>}
   * @private
   */
  getCustomApnList_() {
    return this.managedCellularProperties?.customApnList ?? [];
  }

  /**
   * @returns {boolean}
   * @private
   */
  computeHasEnabledDefaultCustomApn_() {
    return this.getCustomApnList_().some(
        (apn) => apn.state === ApnState.kEnabled && isDefaultApn(apn));
  }

  /**
   * @returns {Array<ApnProperties>}
   * @private
   */
  getValidDatabaseApnList_() {
    const databaseApnList =
        this.managedCellularProperties?.apnList?.activeValue ?? [];
    return databaseApnList.filter((apn) => {
      if (apn.source !== ApnSource.kModb) {
        return false;
      }

      // Only APNs that have type default are allowed unless an enabled custom
      // APN of type default already exists. In that case, APNs that are of type
      // attach are also permitted.
      return isDefaultApn(apn) ||
          (this.hasEnabledDefaultCustomApn_ && isAttachApn(apn));
    });
  }
}

customElements.define(ApnList.is, ApnList);
