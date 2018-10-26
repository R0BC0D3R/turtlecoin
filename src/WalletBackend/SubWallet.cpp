// Copyright (c) 2018, The TurtleCoin Developers
// 
// Please see the included LICENSE file for more information.

////////////////////////////////////
#include <WalletBackend/SubWallet.h>
////////////////////////////////////

#include <CryptoNoteCore/Account.h>
#include <CryptoNoteCore/CryptoNoteBasicImpl.h>

#include <WalletBackend/Utilities.h>

//////////////////////////
/* NON MEMBER FUNCTIONS */
//////////////////////////

namespace {
} // namespace

///////////////////////////////////
/* CONSTRUCTORS / DECONSTRUCTORS */
///////////////////////////////////

SubWallet::SubWallet()
{
}

/* Makes a view only subwallet */
SubWallet::SubWallet(const Crypto::PublicKey publicSpendKey,
                     const std::string address,
                     const uint64_t scanHeight, const uint64_t scanTimestamp) :
    m_publicSpendKey(publicSpendKey),
    m_address(address),
    m_syncStartHeight(scanHeight),
    m_syncStartTimestamp(scanTimestamp),
    m_isViewWallet(true)
{
}

/* Makes a standard subwallet */
SubWallet::SubWallet(const Crypto::PublicKey publicSpendKey,
                     const Crypto::SecretKey privateSpendKey,
                     const std::string address,
                     const uint64_t scanHeight, const uint64_t scanTimestamp) :

    m_publicSpendKey(publicSpendKey),
    m_address(address),
    m_syncStartHeight(scanHeight),
    m_syncStartTimestamp(scanTimestamp),
    m_isViewWallet(false),
    m_privateSpendKey(privateSpendKey)
{
}

/////////////////////
/* CLASS FUNCTIONS */
/////////////////////

void SubWallet::completeAndStoreTransactionInput(
    const Crypto::KeyDerivation derivation,
    const size_t outputIndex,
    WalletTypes::TransactionInput input)
{
    if (m_isViewWallet)
    {
        throw std::invalid_argument(
            "Attempted to generate key image from view only subwallet!"
        );
    }

    Crypto::KeyImage keyImage;

    /* Make a temporary key pair */
    CryptoNote::KeyPair tmp;

    /* Get the tmp public key from the derivation, the index,
       and our public spend key */
    Crypto::derive_public_key(
        derivation, outputIndex, m_publicSpendKey, tmp.publicKey
    );

    /* Get the tmp private key from the derivation, the index,
       and our private spend key */
    Crypto::derive_secret_key(
        derivation, outputIndex, m_privateSpendKey, tmp.secretKey
    );

    /* Get the key image from the tmp public and private key */
    Crypto::generate_key_image(
        tmp.publicKey, tmp.secretKey, keyImage
    );

    input.keyImage = keyImage;

    m_transactionInputs.push_back(input);
}

std::tuple<uint64_t, uint64_t> SubWallet::getBalance(
    const uint64_t currentHeight) const
{
    uint64_t unlockedBalance = 0;

    uint64_t lockedBalance = 0;

    for (const auto input : m_transactionInputs)
    {
        /* In transaction pool, or has been spent already */
        if (input.isLocked || input.isSpent)
        {
            continue;
        }

        /* If an unlock height is present, check if the input is unlocked */
        if (Utilities::isInputUnlocked(input.unlockTime, currentHeight))
        {
            unlockedBalance += input.amount;
        }
        else
        {
            lockedBalance += input.amount;
        }
    }

    return {unlockedBalance, lockedBalance};
}