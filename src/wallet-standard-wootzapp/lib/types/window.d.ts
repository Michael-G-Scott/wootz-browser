import { type SolanaSignInInput, type SolanaSignInOutput } from '@solana/wallet-standard-features';
import type { PublicKey, SendOptions, Transaction, TransactionSignature, VersionedTransaction } from '@solana/web3.js';
export interface WootzappWalletEvent {
    connect(...args: unknown[]): unknown;
    disconnect(...args: unknown[]): unknown;
    accountChanged(...args: unknown[]): unknown;
}
export interface WootzappWalletEventEmitter {
    on<E extends keyof WootzappWalletEvent>(event: E, listener: WootzappWalletEvent[E], context?: any): void;
    off<E extends keyof WootzappWalletEvent>(event: E, listener: WootzappWalletEvent[E], context?: any): void;
}
export interface WootzappWallet extends WootzappWalletEventEmitter {
    publicKey: PublicKey | null;
    connect(options?: {
        onlyIfTrusted?: boolean;
    }): Promise<{
        publicKey: PublicKey;
    }>;
    disconnect(): Promise<void>;
    signAndSendTransaction<T extends Transaction | VersionedTransaction>(transaction: T, options?: SendOptions): Promise<{
        signature: TransactionSignature;
    }>;
    signTransaction<T extends Transaction | VersionedTransaction>(transaction: T): Promise<T>;
    signAllTransactions<T extends Transaction | VersionedTransaction>(transactions: T[]): Promise<T[]>;
    signMessage(message: Uint8Array): Promise<{
        signature: Uint8Array;
    }>;
    signIn(input?: SolanaSignInInput): Promise<SolanaSignInOutput>;
}
//# sourceMappingURL=window.d.ts.map